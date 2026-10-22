/* rros-security: audita la seguridad del equipo y aplica niveles de seguridad.
 *   rros-security [auditoria] [--json]     estado con puntuación y cómo arreglar cada punto
 *   rros-security niveles                  lista los niveles
 *   rros-security nivel NOMBRE [--dry-run] aplica un nivel (root)
 *   rros-security abrir|cerrar PUERTO      abre o cierra un puerto del cortafuegos (root)
 * RROS_ROOT antepone un directorio a /proc, /sys y /etc (para las pruebas). */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "rros_profile.h"
#include "rros_schema.h"
#include "rros_security.h"
#include "rros_userconf.h"

static const char *root = "";

enum st { OK, WARN, FAIL, INFO };

struct check {
	const char *id;
	enum st st;
	char title[64], detail[640], fix[200];
};

static struct check checks[24];
static int nchecks;

static struct check *add(const char *id, enum st st, const char *title)
{
	struct check *c = &checks[nchecks < 23 ? nchecks++ : 23];

	memset(c, 0, sizeof *c);
	c->id = id;
	c->st = st;
	snprintf(c->title, sizeof c->title, "%s", title);
	return c;
}

static int read_file(const char *rel, char *buf, size_t n)
{
	char path[600];
	FILE *f;
	size_t got;

	snprintf(path, sizeof path, "%s%s", root, rel);
	f = fopen(path, "r");
	if (!f)
		return -1;
	got = fread(buf, 1, n - 1, f);
	buf[got] = '\0';
	fclose(f);
	return 0;
}

static int exists(const char *rel)
{
	char path[600];

	snprintf(path, sizeof path, "%s%s", root, rel);
	return access(path, F_OK) == 0;
}

static int unit_active(const char *unit)
{
	pid_t pid = fork();
	int st;

	if (pid < 0)
		return 0;
	if (pid == 0) {
		if (!freopen("/dev/null", "w", stdout) || !freopen("/dev/null", "w", stderr))
			_exit(127);
		execlp("systemctl", "systemctl", "is-active", "--quiet", unit, (char *)NULL);
		_exit(127);
	}
	if (waitpid(pid, &st, 0) < 0)
		return 0;
	return WIFEXITED(st) && WEXITSTATUS(st) == 0;
}

static void trim(char *s)
{
	size_t n = strlen(s);

	while (n && isspace((unsigned char)s[n - 1]))
		s[--n] = '\0';
}

static int is_live(void)
{
	char cmd[1024] = "";

	return read_file("/proc/cmdline", cmd, sizeof cmd) == 0 &&
	       (strstr(cmd, "boot=casper") != NULL || strstr(cmd, "archisobasedir=") != NULL);
}

/* ---- puertos abiertos ---- */

struct port { char proto[5]; unsigned port; };

static int port_ignored(const char *proto, unsigned p)
{
	/* clientes DHCP y mDNS: escuchan respuestas, no ofrecen servicios */
	return !strncmp(proto, "udp", 3) && (p == 68 || p == 546 || p == 5353);
}

static int collect(const char *file, const char *proto, int v6, struct port *out, int n, int max)
{
	char path[600], line[512];
	FILE *f;

	snprintf(path, sizeof path, "%s%s", root, file);
	f = fopen(path, "r");
	if (!f)
		return n;
	if (!fgets(line, sizeof line, f)) {
		fclose(f);
		return n;
	}
	while (fgets(line, sizeof line, f) && n < max) {
		char local[64], remote[64], state[8];
		char *colon;
		unsigned port;
		int loop, dup = 0;

		if (sscanf(line, "%*s %63s %63s %7s", local, remote, state) != 3)
			continue;
		/* TCP: 0A = LISTEN; UDP: 07 = sin conectar (esperando datos) */
		if (strcmp(state, proto[0] == 't' ? "0A" : "07"))
			continue;
		colon = strchr(local, ':');
		if (!colon)
			continue;
		*colon = '\0';
		port = (unsigned)strtoul(colon + 1, NULL, 16);
		loop = v6 ? !strcmp(local, "00000000000000000000000001000000") ||
			    !strncmp(local, "0000000000000000FFFF00000", 25)
			  : !strncmp(local + 6, "7F", 2);
		if (loop || port_ignored(proto, port))
			continue;
		for (int i = 0; i < n; i++)
			if (out[i].port == port && !strncmp(out[i].proto, proto, 3))
				dup = 1;
		if (dup)
			continue;
		snprintf(out[n].proto, sizeof out[n].proto, "%.3s", proto);
		out[n].port = port;
		n++;
	}
	fclose(f);
	return n;
}

/* ---- comprobaciones ---- */

static void audit(const struct rros_conf *eff)
{
	struct rros_secpol sp;
	struct check *c;
	char buf[8192];
	int live = is_live(), arch = rros_is_arch(root);

	rros_secpol_from_conf(&sp, eff);
	nchecks = 0;

	/* cortafuegos */
	if (!sp.firewall) {
		c = add("firewall", FAIL, "Cortafuegos");
		snprintf(c->detail, sizeof c->detail, "Desactivado: cualquier programa que escuche en la red es accesible.");
		snprintf(c->fix, sizeof c->fix, "sudo rros-security nivel estandar");
	} else if (read_file("/etc/nftables.conf", buf, sizeof buf) || !strstr(buf, "policy drop")) {
		c = add("firewall", WARN, "Cortafuegos");
		snprintf(c->detail, sizeof c->detail, "Activado en los ajustes pero las reglas aún no se han generado.");
		snprintf(c->fix, sizeof c->fix, "sudo rros-apply --services");
	} else if (!unit_active("nftables.service")) {
		c = add("firewall", WARN, "Cortafuegos");
		snprintf(c->detail, sizeof c->detail, "Reglas listas, pero nftables no está en marcha.");
		snprintf(c->fix, sizeof c->fix, "sudo systemctl enable --now nftables");
	} else {
		c = add("firewall", OK, "Cortafuegos");
		snprintf(c->detail, sizeof c->detail, "Activo: se bloquean las conexiones entrantes no pedidas%s%s%s.",
			 sp.allow[0] ? " (abiertos: " : "", sp.allow, sp.allow[0] ? ")" : "");
	}

	/* puertos */
	{
		struct port ports[64];
		int n = 0;
		char list[256] = "";

		n = collect("/proc/net/tcp", "tcp", 0, ports, n, 64);
		n = collect("/proc/net/tcp6", "tcp", 1, ports, n, 64);
		n = collect("/proc/net/udp", "udp", 0, ports, n, 64);
		n = collect("/proc/net/udp6", "udp", 1, ports, n, 64);
		for (int i = 0; i < n; i++) {
			char one[24];

			snprintf(one, sizeof one, "%s%u/%.3s", i ? ", " : "", ports[i].port, ports[i].proto);
			if (strlen(list) + strlen(one) + 1 < sizeof list)
				strcat(list, one);
		}
		if (n == 0) {
			c = add("ports", OK, "Programas escuchando en la red");
			snprintf(c->detail, sizeof c->detail, "Ninguno: nada acepta conexiones desde fuera del equipo.");
		} else if (!sp.firewall) {
			c = add("ports", FAIL, "Programas escuchando en la red");
			snprintf(c->detail, sizeof c->detail, "%d puerto(s) abiertos sin cortafuegos: %s.", n, list);
			snprintf(c->fix, sizeof c->fix, "sudo rros-security nivel estandar");
		} else {
			c = add("ports", INFO, "Programas escuchando en la red");
			snprintf(c->detail, sizeof c->detail, "%d puerto(s): %s. El cortafuegos solo deja pasar los permitidos.",
				 n, list);
		}
	}

	/* kernel */
	{
		int bad = 0, total = 0;
		char missing[200] = "";

		for (size_t i = 0; i < rros_hardening_len; i++) {
			char rel[200], val[64];

			snprintf(rel, sizeof rel, "/proc/sys/%s", rros_hardening[i].key);
			for (char *p = rel + 10; *p; p++)
				if (*p == '.')
					*p = '/';
			if (read_file(rel, val, sizeof val))
				continue;   /* el kernel no tiene ese ajuste */
			trim(val);
			total++;
			if (strcmp(val, rros_hardening[i].value)) {
				bad++;
				if (strlen(missing) + strlen(rros_hardening[i].key) + 3 < sizeof missing)
					snprintf(missing + strlen(missing), sizeof missing - strlen(missing), "%s%s",
						 *missing ? ", " : "", rros_hardening[i].key);
			}
		}
		if (!sp.hardening) {
			c = add("kernel", WARN, "Kernel endurecido");
			snprintf(c->detail, sizeof c->detail, "Desactivado (security.kernel_hardening = false).");
			snprintf(c->fix, sizeof c->fix, "sudo rros-config set --system security.kernel_hardening true && sudo rros-apply --services");
		} else if (bad) {
			c = add("kernel", WARN, "Kernel endurecido");
			snprintf(c->detail, sizeof c->detail, "%d de %d ajustes sin aplicar: %s.", bad, total, missing);
			snprintf(c->fix, sizeof c->fix, "sudo rros-apply --services");
		} else {
			c = add("kernel", OK, "Kernel endurecido");
			snprintf(c->detail, sizeof c->detail, "%d ajustes aplicados (punteros, dmesg, BPF, ptrace, redirecciones).", total);
		}
	}

	/* AppArmor */
	if (read_file("/sys/module/apparmor/parameters/enabled", buf, 16) == 0 && buf[0] == 'Y') {
		int n = -1;
		char *prof = malloc(1 << 16);

		if (prof && read_file("/sys/kernel/security/apparmor/profiles", prof, 1 << 16) == 0) {
			n = 0;
			for (char *p = prof; (p = strchr(p, '\n')); p++)
				n++;
		}
		free(prof);
		if (n == 0 && live) {
			/* en el sistema live no se cargan perfiles (apparmor.service se lo salta) */
			c = add("apparmor", INFO, "AppArmor");
			snprintf(c->detail, sizeof c->detail, "Activo en el kernel; en el sistema live no se cargan perfiles (al instalar, sí).");
		} else if (n == 0) {
			c = add("apparmor", WARN, "AppArmor");
			snprintf(c->detail, sizeof c->detail, "Activo pero sin perfiles cargados: ningún programa está confinado.");
			snprintf(c->fix, sizeof c->fix, "sudo systemctl enable --now apparmor");
		} else {
			c = add("apparmor", OK, "AppArmor");
			if (n > 0)
				snprintf(c->detail, sizeof c->detail, "Activo con %d perfiles de confinamiento cargados.", n);
			else
				snprintf(c->detail, sizeof c->detail, "Activo en el kernel (el número de perfiles solo lo ve root).");
		}
	} else {
		c = add("apparmor", WARN, "AppArmor");
		snprintf(c->detail, sizeof c->detail, "No está activo: los programas no están confinados.");
		snprintf(c->fix, sizeof c->fix, "sudo pacman -S apparmor y añade lsm=landlock,lockdown,yama,integrity,apparmor,bpf al arranque");
	}

	/* actualizaciones: se descargan cada día (rros-updates.timer)
	 * y se instalan cuando tú quieras, porque en Arch actualizar sin mirar puede romper el sistema */
	if (arch && exists("/etc/systemd/system/timers.target.wants/rros-updates.timer")) {
		c = add("updates", OK, "Actualizaciones");
		snprintf(c->detail, sizeof c->detail, "Se descargan solas cada día; instálalas con «rros apps update» (o desde Ajustes).");
	} else if (arch) {
		c = add("updates", WARN, "Actualizaciones");
		snprintf(c->detail, sizeof c->detail, sp.auto_updates ? "Activadas en los ajustes, pero falta aplicarlas." :
			 "Desactivadas: comprueba tú si hay actualizaciones (rros apps update).");
		snprintf(c->fix, sizeof c->fix, "sudo rros-config set --system system.auto_updates true && sudo rros-apply --services");
	} else if (read_file("/etc/apt/apt.conf.d/20auto-upgrades", buf, sizeof buf) == 0 &&
	    strstr(buf, "Unattended-Upgrade \"1\"") && exists("/usr/bin/unattended-upgrade")) {
		c = add("updates", OK, "Actualizaciones de seguridad");
		snprintf(c->detail, sizeof c->detail, "Se instalan solas cada día.");
	} else {
		c = add("updates", WARN, "Actualizaciones de seguridad");
		snprintf(c->detail, sizeof c->detail, sp.auto_updates ? "Activadas en los ajustes, pero falta aplicarlas o instalar unattended-upgrades." :
			 "Desactivadas: tendrás que actualizar a mano (rros apps update).");
		snprintf(c->fix, sizeof c->fix, "sudo rros-config set --system system.auto_updates true && sudo rros-apply --services");
	}

	/* intentos de contraseña */
	{
		int pam = read_file(arch ? "/etc/pam.d/system-auth" : "/etc/pam.d/common-auth", buf, sizeof buf) == 0 &&
			  strstr(buf, "pam_faillock.so");

		if (sp.login_attempts > 0 && pam) {
			c = add("faillock", OK, "Bloqueo por contraseñas fallidas");
			snprintf(c->detail, sizeof c->detail, "Tras %d intentos fallidos la cuenta se bloquea 10 minutos.", sp.login_attempts);
		} else if (sp.login_attempts > 0) {
			c = add("faillock", WARN, "Bloqueo por contraseñas fallidas");
			snprintf(c->detail, sizeof c->detail, "Configurado (%d intentos) pero aún no aplicado.", sp.login_attempts);
			snprintf(c->fix, sizeof c->fix, "sudo rros-apply --services");
		} else {
			c = add("faillock", WARN, "Bloqueo por contraseñas fallidas");
			snprintf(c->detail, sizeof c->detail, "Sin límite: se pueden probar contraseñas sin parar.");
			snprintf(c->fix, sizeof c->fix, "sudo rros-config set --system security.login_attempts 10 && sudo rros-apply --services");
		}
	}

	/* sudo */
	{
		int t = -1;
		char *p;

		if (read_file("/etc/sudoers.d/rros", buf, sizeof buf) == 0 && (p = strstr(buf, "timestamp_timeout=")))
			t = atoi(p + 18);
		c = add("sudo", t >= 0 && t <= 5 ? OK : INFO, "sudo");
		if (t == 0)
			snprintf(c->detail, sizeof c->detail, "Pide la contraseña cada vez.");
		else if (t > 0)
			snprintf(c->detail, sizeof c->detail, "Recuerda la contraseña %d minuto(s).", t);
		else
			snprintf(c->detail, sizeof c->detail, "Valor por defecto (15 minutos).");
	}

	/* bloqueo de pantalla */
	{
		long m = rros_conf_get_long(eff, "privacy.lock_minutes", 10);   /* mismo valor por defecto que rros-theme */

		c = add("lock", m > 0 && m <= 15 ? OK : WARN, "Bloqueo de pantalla");
		if (m > 0)
			snprintf(c->detail, sizeof c->detail, "Se bloquea tras %ld minuto(s) sin uso.", m);
		else
			snprintf(c->detail, sizeof c->detail, "Nunca se bloquea sola.");
		if (m <= 0 || m > 15)
			snprintf(c->fix, sizeof c->fix, "rros-config set privacy.lock_minutes 10");
	}

	/* cuentas */
	{
		int extra_root = 0, empty = -1;
		char *line, *save = NULL;

		if (read_file("/etc/passwd", buf, sizeof buf) == 0)
			for (line = strtok_r(buf, "\n", &save); line; line = strtok_r(NULL, "\n", &save)) {
				char name[64];
				int uid;

				if (sscanf(line, "%63[^:]:%*[^:]:%d:", name, &uid) == 2 && uid == 0 && strcmp(name, "root"))
					extra_root++;
			}
		if (read_file("/etc/shadow", buf, sizeof buf) == 0) {
			empty = 0;
			save = NULL;
			for (line = strtok_r(buf, "\n", &save); line; line = strtok_r(NULL, "\n", &save)) {
				char *colon = strchr(line, ':');

				if (colon && colon[1] == ':')
					empty++;
			}
		}
		if (extra_root) {
			c = add("accounts", FAIL, "Cuentas");
			snprintf(c->detail, sizeof c->detail, "%d cuenta(s) además de root tienen UID 0 (poder total).", extra_root);
			snprintf(c->fix, sizeof c->fix, "revisa /etc/passwd");
		} else if (empty > 0 && !live) {
			c = add("accounts", FAIL, "Cuentas");
			snprintf(c->detail, sizeof c->detail, "%d cuenta(s) sin contraseña.", empty);
			snprintf(c->fix, sizeof c->fix, "sudo passwd USUARIO");
		} else {
			c = add("accounts", empty < 0 ? INFO : OK, "Cuentas");
			snprintf(c->detail, sizeof c->detail, empty < 0 ? "Solo root tiene UID 0 (las contraseñas vacías solo las ve root: usa sudo)." :
				 live ? "Solo root tiene UID 0. Sistema live: el usuario no tiene contraseña (normal)." :
				 "Solo root tiene UID 0 y todas las cuentas tienen contraseña.");
		}
	}

	/* SSH */
	if (unit_active(arch ? "sshd.service" : "ssh.service")) {
		c = add("ssh", sp.ssh ? INFO : WARN, "Servidor SSH");
		snprintf(c->detail, sizeof c->detail, "En marcha: se puede entrar a este equipo por la red (puerto 22).");
		snprintf(c->fix, sizeof c->fix, "sudo rros-config set --system services.ssh false && sudo rros-apply --services");
	} else {
		c = add("ssh", OK, "Servidor SSH");
		snprintf(c->detail, sizeof c->detail, "No hay servidor SSH en marcha.");
	}

	/* cifrado */
	if (live) {
		c = add("encryption", INFO, "Cifrado del disco");
		snprintf(c->detail, sizeof c->detail, "Sistema live: no guarda nada en disco.");
	} else {
		char dev[128] = "", *save = NULL, *line;
		int crypt = 0;

		if (read_file("/proc/mounts", buf, sizeof buf) == 0)
			for (line = strtok_r(buf, "\n", &save); line; line = strtok_r(NULL, "\n", &save)) {
				char d[128], mp[128];

				if (sscanf(line, "%127s %127s", d, mp) == 2 && !strcmp(mp, "/"))
					snprintf(dev, sizeof dev, "%s", d);
			}
		if (!strncmp(dev, "/dev/mapper/", 12) || !strncmp(dev, "/dev/dm-", 8)) {
			/* el uuid del dispositivo dm empieza por CRYPT- si es LUKS */
			for (int i = 0; i < 16 && !crypt; i++) {
				char rel[64], uuid[128];

				snprintf(rel, sizeof rel, "/sys/block/dm-%d/dm/uuid", i);
				if (read_file(rel, uuid, sizeof uuid) == 0 && !strncmp(uuid, "CRYPT-", 6))
					crypt = 1;
			}
		}
		c = add("encryption", crypt ? OK : WARN, "Cifrado del disco");
		snprintf(c->detail, sizeof c->detail, crypt ? "El sistema está en un volumen cifrado (LUKS)." :
			 "El disco no está cifrado: quien se lleve el equipo puede leer tus archivos.");
		if (!crypt)
			snprintf(c->fix, sizeof c->fix, "el instalador de Lily aún no cifra; cifra tu carpeta o usa LUKS al instalar");
	}

	/* arranque seguro */
	if (!exists("/sys/firmware/efi")) {
		c = add("secureboot", INFO, "Arranque seguro (Secure Boot)");
		snprintf(c->detail, sizeof c->detail, "El equipo arranca en modo BIOS: no hay Secure Boot.");
	} else {
		unsigned char sb[8] = { 0 };
		char path[600];
		FILE *f;
		int on = 0;

		snprintf(path, sizeof path, "%s/sys/firmware/efi/efivars/SecureBoot-8be4df61-93ca-11d2-aa0d-00e098032b8c", root);
		f = fopen(path, "rb");
		if (f) {
			on = fread(sb, 1, 5, f) == 5 && sb[4] == 1;
			fclose(f);
		}
		c = add("secureboot", on ? OK : WARN, "Arranque seguro (Secure Boot)");
		snprintf(c->detail, sizeof c->detail, on ? "Activo: solo arranca software firmado." : "Desactivado en el firmware.");
		if (!on)
			snprintf(c->fix, sizeof c->fix, "actívalo en la configuración UEFI del equipo");
	}

	/* dispositivos */
	c = add("devices", INFO, "Memorias USB y cámara");
	snprintf(c->detail, sizeof c->detail, "Memorias USB %s, cámara %s.", sp.usb_storage ? "permitidas" : "bloqueadas",
		 sp.camera ? "permitida" : "bloqueada");

	/* volcados */
	if (read_file("/proc/sys/fs/suid_dumpable", buf, 16) == 0) {
		trim(buf);
		c = add("coredumps", !strcmp(buf, "0") ? OK : INFO, "Volcados de memoria");
		snprintf(c->detail, sizeof c->detail, !strcmp(buf, "0") ? "Los programas con privilegios no dejan volcados." :
			 "Los programas con privilegios pueden dejar volcados de memoria.");
	}
}

/* ---- nivel actual ---- */

static const char *current_level(const struct rros_conf *eff)
{
	for (size_t i = 0; i < rros_seclevels_len; i++) {
		int same = 1;

		for (const char *const *kv = rros_seclevels[i].kv; *kv && same; kv++) {
			char key[96];
			const char *eq = strchr(*kv, '='), *val;

			snprintf(key, sizeof key, "%.*s", (int)(eq - *kv), *kv);
			val = rros_conf_get(eff, key);
			if (!val || strcmp(val, eq + 1))
				same = 0;
		}
		if (same)
			return rros_seclevels[i].name;
	}
	return "personalizado";
}

static int score(void)
{
	int ok = 0, all = 0;

	for (int i = 0; i < nchecks; i++) {
		if (checks[i].st == INFO)
			continue;
		all++;
		ok += checks[i].st == OK;
	}
	return all ? (100 * ok + all / 2) / all : 100;
}

static void json_str(const char *s)
{
	putchar('"');
	for (; *s; s++) {
		if (*s == '"' || *s == '\\')
			printf("\\%c", *s);
		else if ((unsigned char)*s < 0x20)
			printf("\\u%04x", *s);
		else
			putchar(*s);
	}
	putchar('"');
}

/* imprime s ocupando `width` columnas (cuenta caracteres UTF-8, no bytes) */
static void put_padded(const char *s, int width)
{
	int cols = 0;

	for (const char *p = s; *p; p++)
		cols += ((unsigned char)*p & 0xC0) != 0x80;
	fputs(s, stdout);
	for (; cols < width; cols++)
		putchar(' ');
}

static const char *st_name(enum st s)
{
	return s == OK ? "ok" : s == WARN ? "warn" : s == FAIL ? "fail" : "info";
}

static int cmd_audit(const struct rros_conf *eff, int json)
{
	const char *lvl = current_level(eff);
	int color = !json && isatty(STDOUT_FILENO) && !getenv("NO_COLOR");

	audit(eff);
	if (json) {
		printf("{\"score\":%d,\"level\":", score());
		json_str(lvl);
		fputs(",\"checks\":[", stdout);
		for (int i = 0; i < nchecks; i++) {
			printf("%s{\"id\":", i ? "," : "");
			json_str(checks[i].id);
			printf(",\"status\":\"%s\",\"title\":", st_name(checks[i].st));
			json_str(checks[i].title);
			fputs(",\"detail\":", stdout);
			json_str(checks[i].detail);
			fputs(",\"fix\":", stdout);
			json_str(checks[i].fix);
			putchar('}');
		}
		puts("]}");
		return 0;
	}
	printf("Lily · seguridad   puntuación %d/100   nivel: %s\n\n", score(), lvl);
	for (int i = 0; i < nchecks; i++) {
		static const char *sym[] = { "✓", "!", "✗", "·" }, *plain[] = { "[ok]", "[!] ", "[x] ", "[i] " };
		static const char *col[] = { "\033[32m", "\033[33m", "\033[31m", "\033[36m" };
		const struct check *c = &checks[i];

		if (color)
			printf("  %s%s\033[0m ", col[c->st], sym[c->st]);
		else
			printf("  %s ", plain[c->st]);
		put_padded(c->title, 33);
		printf("%s\n", c->detail);
		if (*c->fix)
			printf("       %-32s → %s\n", "", c->fix);
	}
	return 0;
}

/* ---- cambios (root) ---- */

static int apply_now(void)
{
	pid_t pid;
	int st;

	fflush(stdout);
	pid = fork();
	if (pid < 0)
		return 1;
	if (pid == 0) {
		execlp("rros-apply", "rros-apply", "--services", (char *)NULL);
		perror("rros-apply");
		_exit(127);
	}
	if (waitpid(pid, &st, 0) < 0)
		return 1;
	return WIFEXITED(st) ? WEXITSTATUS(st) : 1;
}

static int need_root(void)
{
	if (geteuid() == 0 || getenv("RROS_SECURITY_NOROOT"))
		return 0;
	fputs("rros-security: este cambio afecta a todo el sistema; ejecútalo con sudo\n", stderr);
	return 1;
}

static int cmd_level(const char *name, int dry)
{
	const struct rros_seclevel *l = rros_seclevel_find(name);
	char err[300];

	if (!l) {
		fprintf(stderr, "rros-security: nivel desconocido '%s' (usa: rros-security niveles)\n", name);
		return 2;
	}
	printf("Nivel %s: %s\n", l->name, l->desc);
	for (const char *const *kv = l->kv; *kv; kv++)
		printf("  %s\n", *kv);
	if (dry) {
		puts("(simulación: no se ha cambiado nada)");
		return 0;
	}
	if (need_root())
		return 1;
	for (const char *const *kv = l->kv; *kv; kv++) {
		char key[96];
		const char *eq = strchr(*kv, '=');

		snprintf(key, sizeof key, "%.*s", (int)(eq - *kv), *kv);
		if (rros_user_set(key, eq + 1, 1, err, sizeof err)) {
			fprintf(stderr, "rros-security: %s\n", err);
			return 1;
		}
	}
	return getenv("RROS_SECURITY_NOAPPLY") ? 0 : apply_now();
}

/* abrir/cerrar: edita security.firewall_allow en la capa del sistema sin duplicados */
static int cmd_port(const char *profile, const char *spec, int open)
{
	struct rros_conf eff;
	char err[300], norm[32], list[300] = "", out[300] = "";
	const char *cur;
	int found = 0;

	if (rros_fw_ports_check(spec, err, sizeof err) || strchr(spec, ',')) {
		fprintf(stderr, "rros-security: %s\n", *err ? err : "indica un solo puerto");
		return 2;
	}
	snprintf(norm, sizeof norm, strchr(spec, '/') ? "%s" : "%s/tcp", spec);
	if (need_root())
		return 1;
	rros_conf_init(&eff);
	rros_load_effective(&eff, profile);
	cur = rros_conf_get(&eff, "security.firewall_allow");
	snprintf(list, sizeof list, "%s", cur ? cur : "");
	rros_conf_free(&eff);
	for (char *save = NULL, *tok = strtok_r(list, ", ", &save); tok; tok = strtok_r(NULL, ", ", &save)) {
		char t[32];

		snprintf(t, sizeof t, strchr(tok, '/') ? "%s" : "%s/tcp", tok);
		if (!strcmp(t, norm)) {
			found = 1;
			if (!open)
				continue;
		}
		if (strlen(out) + strlen(t) + 2 < sizeof out)
			snprintf(out + strlen(out), sizeof out - strlen(out), "%s%s", *out ? "," : "", t);
	}
	if (open && !found && strlen(out) + strlen(norm) + 2 < sizeof out)
		snprintf(out + strlen(out), sizeof out - strlen(out), "%s%s", *out ? "," : "", norm);
	if (!open && !found) {
		printf("%s no estaba abierto\n", norm);
		return 0;
	}
	if (rros_user_set("security.firewall_allow", out, 1, err, sizeof err)) {
		fprintf(stderr, "rros-security: %s\n", err);
		return 1;
	}
	printf("%s %s. Puertos abiertos: %s\n", norm, open ? "abierto" : "cerrado", *out ? out : "ninguno");
	return getenv("RROS_SECURITY_NOAPPLY") ? 0 : apply_now();
}

static void usage(void)
{
	fputs("uso: rros-security [COMANDO]\n"
	      "  auditoria [--json]         estado de la seguridad, con puntuación (por defecto)\n"
	      "  niveles                    niveles disponibles\n"
	      "  nivel NOMBRE [--dry-run]   aplica un nivel: basico, estandar o estricto (sudo)\n"
	      "  abrir PUERTO               abre un puerto, p. ej. 8080/tcp (sudo)\n"
	      "  cerrar PUERTO              lo vuelve a cerrar (sudo)\n"
	      "Más: docs/SEGURIDAD.md\n", stderr);
}

int main(int argc, char **argv)
{
	char profile[64];
	struct rros_conf eff;
	const char *cmd = argc > 1 ? argv[1] : "auditoria";
	int rc;

	if (getenv("RROS_ROOT"))
		root = getenv("RROS_ROOT");
	rros_read_active_profile(profile, sizeof profile);

	if (!strcmp(cmd, "niveles") || !strcmp(cmd, "levels")) {
		for (size_t i = 0; i < rros_seclevels_len; i++)
			printf("%-10s %s\n", rros_seclevels[i].name, rros_seclevels[i].desc);
		return 0;
	}
	if (!strcmp(cmd, "nivel") || !strcmp(cmd, "level")) {
		if (argc < 3) {
			usage();
			return 2;
		}
		return cmd_level(argv[2], argc > 3 && !strcmp(argv[3], "--dry-run"));
	}
	if (!strcmp(cmd, "abrir") || !strcmp(cmd, "allow") || !strcmp(cmd, "cerrar") || !strcmp(cmd, "deny")) {
		if (argc < 3) {
			usage();
			return 2;
		}
		return cmd_port(profile, argv[2], !strcmp(cmd, "abrir") || !strcmp(cmd, "allow"));
	}
	if (strcmp(cmd, "auditoria") && strcmp(cmd, "audit") && strcmp(cmd, "estado") && strcmp(cmd, "status") &&
	    strcmp(cmd, "--json")) {
		usage();
		return 2;
	}
	rros_conf_init(&eff);
	if (rros_load_effective(&eff, profile)) {
		fprintf(stderr, "rros-security: no se pudo cargar la configuración del perfil '%s'\n", profile);
		return 1;
	}
	rc = cmd_audit(&eff, !strcmp(cmd, "--json") || (argc > 2 && !strcmp(argv[2], "--json")));
	rros_conf_free(&eff);
	return rc;
}
