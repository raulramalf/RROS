/* rros-apply: traduce la configuración efectiva (perfil + ajustes) a ajustes reales del sistema. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "rros_profile.h"

static const struct { const char *key, *unit; } services[] = {
	{ "services.bluetooth", "bluetooth.service" },
	{ "services.printing", "cups.service" },
	{ "services.ssh", "ssh.service" },
	{ "services.location", "geoclue.service" },
};

static const char *root = "";

static int write_text(const char *rel, const char *text)
{
	char path[1100], tmp[1110];
	FILE *f;

	snprintf(path, sizeof path, "%s%s", root, rel);
	snprintf(tmp, sizeof tmp, "%s.tmp", path);
	if (rros_mkdir_parents(path)) {
		fprintf(stderr, "rros-apply: no se pudo crear el directorio de %s\n", path);
		return -1;
	}
	f = fopen(tmp, "w");
	if (!f) {
		fprintf(stderr, "rros-apply: %s: %s\n", tmp, strerror(errno));
		return -1;
	}
	fputs(text, f);
	if (fclose(f) != 0 || rename(tmp, path) != 0) {
		remove(tmp);
		return -1;
	}
	return 0;
}

static int unit_exists(const char *unit)
{
	static const char *dirs[] = { "/lib/systemd/system", "/usr/lib/systemd/system", "/etc/systemd/system" };
	char p[300];

	for (size_t i = 0; i < sizeof dirs / sizeof *dirs; i++) {
		snprintf(p, sizeof p, "%s%s/%s", root, dirs[i], unit);
		if (access(p, F_OK) == 0)
			return 1;
	}
	return 0;
}

static int systemctl(const char *verb, const char *unit)
{
	pid_t pid = fork();
	int st;

	if (pid < 0)
		return -1;
	if (pid == 0) {
		if (!freopen("/dev/null", "w", stdout) || !freopen("/dev/null", "w", stderr))
			_exit(127);
		execlp("systemctl", "systemctl", "--no-block", verb, unit, (char *)NULL);
		_exit(127);
	}
	if (waitpid(pid, &st, 0) < 0)
		return -1;
	return WIFEXITED(st) && WEXITSTATUS(st) == 0 ? 0 : -1;
}

int main(int argc, char **argv)
{
	char profile[64] = "", buf[1024];
	int act_services = 0, bad = 0;
	struct rros_conf c;
	const char *swap, *algo, *sched;
	long swappiness, zram_pct;

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-p") && i + 1 < argc)
			snprintf(profile, sizeof profile, "%s", argv[++i]);
		else if (!strcmp(argv[i], "--root") && i + 1 < argc)
			root = argv[++i];
		else if (!strcmp(argv[i], "--services"))
			act_services = 1;
		else {
			fputs("uso: rros-apply [-p PERFIL] [--root DIR] [--services]\n", stderr);
			return 2;
		}
	}
	if (!*profile)
		rros_read_active_profile(profile, sizeof profile);
	rros_conf_init(&c);
	if (rros_load_effective(&c, profile)) {
		fprintf(stderr, "rros-apply: no se pudo cargar la configuración del perfil '%s'\n", profile);
		return 1;
	}

	swappiness = rros_conf_get_long(&c, "system.swappiness", 60);
	snprintf(buf, sizeof buf, "# Generado por rros-apply (perfil %s)\nvm.swappiness = %ld\n", profile, swappiness);
	bad |= write_text("/etc/sysctl.d/90-rros-profile.conf", buf) != 0;

	swap = rros_conf_get(&c, "system.swap");
	algo = rros_conf_get(&c, "system.zram_algo");
	zram_pct = rros_conf_get_long(&c, "system.zram_percent", 50);
	if (swap && !strcmp(swap, "zram"))
		snprintf(buf, sizeof buf,
			 "# Generado por rros-apply (perfil %s)\n[zram0]\nzram-size = ram * %ld / 100\n"
			 "compression-algorithm = %s\nswap-priority = 100\n", profile, zram_pct, algo ? algo : "zstd");
	else
		snprintf(buf, sizeof buf, "# Generado por rros-apply: swap en zram desactivado (system.swap = %s)\n",
			 swap ? swap : "none");
	bad |= write_text("/etc/systemd/zram-generator.conf", buf) != 0;

	sched = rros_conf_get(&c, "system.io_scheduler");
	if (!sched || !strcmp(sched, "auto"))
		snprintf(buf, sizeof buf,
			 "# Generado por rros-apply: SSD/NVMe sin planificador, discos giratorios con mq-deadline\n"
			 "ACTION==\"add|change\", KERNEL==\"sd[a-z]|nvme[0-9]n[0-9]|mmcblk[0-9]\", ATTR{queue/rotational}==\"0\", ATTR{queue/scheduler}=\"none\"\n"
			 "ACTION==\"add|change\", KERNEL==\"sd[a-z]\", ATTR{queue/rotational}==\"1\", ATTR{queue/scheduler}=\"mq-deadline\"\n");
	else
		snprintf(buf, sizeof buf,
			 "# Generado por rros-apply\nACTION==\"add|change\", KERNEL==\"sd[a-z]|nvme[0-9]n[0-9]|mmcblk[0-9]\", ATTR{queue/scheduler}=\"%s\"\n", sched);
	bad |= write_text("/etc/udev/rules.d/60-rros-ioscheduler.rules", buf) != 0;

	snprintf(buf, sizeof buf,
		 "# Generado por rros-apply\n[connection]\nwifi.powersave = %d\nwifi.cloned-mac-address = %s\n",
		 rros_conf_get_bool(&c, "net.wifi_powersave", 0) ? 3 : 2,
		 rros_conf_get_bool(&c, "net.mac_randomize", 0) ? "random" : "preserve");
	bad |= write_text("/etc/NetworkManager/conf.d/90-rros.conf", buf) != 0;

	for (size_t i = 0; i < sizeof services / sizeof *services; i++) {
		int on = rros_conf_get_bool(&c, services[i].key, 0);
		const char *verb = on ? "enable" : "disable";

		if (!unit_exists(services[i].unit)) {
			printf("%-22s no instalado\n", services[i].unit);
			continue;
		}
		if (act_services && !*root)
			printf("%-22s %s%s\n", services[i].unit, on ? "activado" : "desactivado",
			       systemctl(verb, services[i].unit) == 0 ? "" : " (falló)");
		else
			printf("%-22s %s (simulado)\n", services[i].unit, on ? "se activaría" : "se desactivaría");
	}
	rros_conf_free(&c);
	printf("perfil %s aplicado%s\n", profile, *root ? " (raíz alternativa)" : "");
	return bad ? 1 : 0;
}
