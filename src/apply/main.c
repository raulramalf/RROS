/* rros-apply: traduce la configuración efectiva (perfil + ajustes) a ajustes reales del sistema. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "rros_profile.h"
#include "rros_security.h"

static const struct { const char *key, *unit; } services[] = {
	{ "services.bluetooth", "bluetooth.service" },
	{ "services.printing", "cups.service" },
	{ "services.ssh", "ssh.service" },
	{ "services.location", "geoclue.service" },
};

static const char *root = "";

static int unit_exists(const char *unit);

static int write_text_mode(const char *rel, const char *text, int mode)
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
	if (fclose(f) != 0 || (mode && chmod(tmp, (mode_t)mode) != 0) || rename(tmp, path) != 0) {
		remove(tmp);
		return -1;
	}
	return 0;
}

static int write_text(const char *rel, const char *text)
{
	return write_text_mode(rel, text, 0);
}

/* Ejecuta una orden sin shell y devuelve su código de salida (-1 si no se pudo ejecutar). */
static int run_cmd(const char *const argv[])
{
	pid_t pid = fork();
	int st;

	if (pid < 0)
		return -1;
	if (pid == 0) {
		if (!freopen("/dev/null", "w", stdout) || !freopen("/dev/null", "w", stderr))
			_exit(127);
		setenv("DEBIAN_FRONTEND", "noninteractive", 1);
		execvp(argv[0], (char *const *)argv);
		_exit(127);
	}
	if (waitpid(pid, &st, 0) < 0)
		return -1;
	return WIFEXITED(st) ? WEXITSTATUS(st) : -1;
}

typedef void (*renderer)(FILE *, const struct rros_secpol *);

static int write_rendered(const char *rel, renderer fn, const struct rros_secpol *p, int mode)
{
	char *text = NULL;
	size_t len = 0;
	FILE *m = open_memstream(&text, &len);
	int rc;

	if (!m)
		return -1;
	fn(m, p);
	fclose(m);
	rc = write_text_mode(rel, text, mode);
	free(text);
	return rc;
}

/* sudoers: se comprueba con visudo antes de instalarlo; un sudoers roto deja sin sudo a todos. */
static int write_sudoers(const struct rros_secpol *p)
{
	char *text = NULL, path[1100], tmp[1120];
	size_t len = 0;
	FILE *m = open_memstream(&text, &len), *f;
	int rc = -1;

	if (!m)
		return -1;
	rros_render_sudoers(m, p);
	fclose(m);
	snprintf(path, sizeof path, "%s/etc/sudoers.d/rros", root);
	snprintf(tmp, sizeof tmp, "%s/etc/sudoers.d/.rros.tmp", root);
	if (rros_mkdir_parents(path) == 0 && (f = fopen(tmp, "w"))) {
		const char *check[] = { "visudo", "-cqf", tmp, NULL };
		int ok;

		fputs(text, f);
		ok = fclose(f) == 0 && chmod(tmp, 0440) == 0;
		/* sin visudo (raíz alternativa sin sudo instalado) se instala igual: el contenido es fijo */
		if (ok && run_cmd(check) > 0) {
			fprintf(stderr, "rros-apply: visudo rechazó el archivo de sudo; no se instala\n");
			ok = 0;
		}
		if (ok && rename(tmp, path) == 0)
			rc = 0;
		else
			remove(tmp);
	}
	free(text);
	return rc;
}

static void apply_security(const struct rros_secpol *sp, int live, int *bad)
{
	*bad |= write_rendered("/etc/nftables.conf", rros_render_nftables, sp, 0755) != 0;
	*bad |= write_rendered("/etc/sysctl.d/80-rros-security.conf", rros_render_hardening, sp, 0) != 0;
	*bad |= write_rendered("/etc/modprobe.d/rros-security.conf", rros_render_modprobe, sp, 0) != 0;
	*bad |= write_sudoers(sp) != 0;
	*bad |= write_rendered("/etc/security/faillock.conf", rros_render_faillock, sp, 0) != 0;
	*bad |= write_rendered("/etc/apt/apt.conf.d/20auto-upgrades", rros_render_autoupdates, sp, 0) != 0;
	*bad |= write_rendered("/etc/security/limits.d/90-rros-core.conf", rros_render_corelimit, sp, 0) != 0;

	if (!live) {
		printf("%-22s %s (simulado)\n", "cortafuegos", sp->firewall ? "se activaría" : "se desactivaría");
		printf("%-22s %s (simulado)\n", "bloqueo por intentos",
		       sp->login_attempts > 0 ? "se activaría" : "se desactivaría");
		return;
	}
	{
		const char *en[] = { "systemctl", "enable", "nftables.service", NULL };
		const char *re[] = { "systemctl", "--no-block", "restart", "nftables.service", NULL };
		const char *sc[] = { "sysctl", "-q", "-p", "/etc/sysctl.d/80-rros-security.conf", NULL };
		const char *pam_on[] = { "pam-auth-update", "--enable", "rros-faillock", "rros-faillock-preauth", NULL };
		const char *pam_off[] = { "pam-auth-update", "--disable", "rros-faillock", "rros-faillock-preauth", NULL };
		const char *rm_usb[] = { "modprobe", "-r", "uas", "usb_storage", NULL };
		const char *rm_cam[] = { "modprobe", "-r", "uvcvideo", NULL };
		int fw = unit_exists("nftables.service") && run_cmd(en) == 0 && run_cmd(re) == 0;

		printf("%-22s %s\n", "cortafuegos", !unit_exists("nftables.service") ? "nftables no instalado" :
		       !fw ? "falló" : sp->firewall ? "activado" : "desactivado");
		if (sp->hardening)
			printf("%-22s %s\n", "kernel endurecido", run_cmd(sc) == 0 ? "aplicado" : "aplicado en parte");
		printf("%-22s %s\n", "bloqueo por intentos",
		       run_cmd(sp->login_attempts > 0 ? pam_on : pam_off) == 0 ?
		       (sp->login_attempts > 0 ? "activado" : "desactivado") : "falló (pam-auth-update)");
		/* los módulos ya cargados se quitan si nadie los usa; si no, el bloqueo vale al reiniciar */
		if (!sp->usb_storage)
			run_cmd(rm_usb);
		if (!sp->camera)
			run_cmd(rm_cam);
	}
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

	{
		struct rros_secpol sp;

		rros_secpol_from_conf(&sp, &c);
		apply_security(&sp, act_services && !*root, &bad);
	}

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
