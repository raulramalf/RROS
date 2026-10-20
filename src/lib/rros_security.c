#include "rros_security.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void rros_secpol_from_conf(struct rros_secpol *p, const struct rros_conf *c)
{
	const char *allow = rros_conf_get(c, "security.firewall_allow");

	memset(p, 0, sizeof *p);
	p->firewall = rros_conf_get_bool(c, "security.firewall", 1);
	p->ping = rros_conf_get_bool(c, "security.firewall_ping", 1);
	p->hardening = rros_conf_get_bool(c, "security.kernel_hardening", 1);
	p->usb_storage = rros_conf_get_bool(c, "security.usb_storage", 1);
	p->camera = rros_conf_get_bool(c, "security.camera", 1);
	p->login_attempts = (int)rros_conf_get_long(c, "security.login_attempts", 0);
	p->sudo_timeout = (int)rros_conf_get_long(c, "security.sudo_timeout_minutes", 15);
	p->core_dumps = rros_conf_get_bool(c, "security.core_dumps", 0);
	p->auto_updates = rros_conf_get_bool(c, "system.auto_updates", 0);
	p->ssh = rros_conf_get_bool(c, "services.ssh", 0);
	p->printing = rros_conf_get_bool(c, "services.printing", 0);
	p->av_realtime = rros_conf_get_bool(c, "security.av_realtime", 0);
	snprintf(p->av_schedule, sizeof p->av_schedule, "%s", rros_conf_get(c, "security.av_schedule") ? rros_conf_get(c, "security.av_schedule") : "semanal");
	if (strcmp(p->av_schedule, "off") && strcmp(p->av_schedule, "diario") && strcmp(p->av_schedule, "semanal"))
		snprintf(p->av_schedule, sizeof p->av_schedule, "semanal");
	p->av_quarantine = !rros_conf_get(c, "security.av_action") || strcmp(rros_conf_get(c, "security.av_action"), "avisar") != 0;
	p->av_updates = rros_conf_get_bool(c, "security.av_updates", 1);
	p->av_max_mb = (int)rros_conf_get_long(c, "security.av_max_file_mb", 100);
	if (p->av_max_mb < 5 || p->av_max_mb > 4000)
		p->av_max_mb = 100;
	p->pw_min = (int)rros_conf_get_long(c, "security.password_min_length", 0);
	if (p->pw_min < 0 || p->pw_min > 64)
		p->pw_min = 0;
	p->pw_complex = rros_conf_get_bool(c, "security.password_complex", 0);
	p->usb_guard = rros_conf_get_bool(c, "security.usb_guard", 0);
	p->tmp_noexec = rros_conf_get_bool(c, "security.tmp_noexec", 0);
	/* una lista inválida no llega nunca al cortafuegos (el esquema ya la rechaza al guardarla) */
	if (allow && rros_fw_ports_check(allow, NULL, 0) == 0)
		snprintf(p->allow, sizeof p->allow, "%s", allow);
}

/* ---- lista de puertos ---- */

struct port_rule { long lo, hi; char proto[4]; };

/* Lee la siguiente regla de *s (avanza el puntero). 1 = regla, 0 = fin, -1 = error. */
static int next_rule(const char **s, struct port_rule *r, char *err, size_t en)
{
	const char *p = *s;
	char *end;

	while (*p == ' ' || *p == ',')
		p++;
	if (!*p)
		return 0;
	if (!isdigit((unsigned char)*p))
		goto bad;
	r->lo = strtol(p, &end, 10);
	r->hi = r->lo;
	p = end;
	if (*p == '-') {
		if (!isdigit((unsigned char)p[1]))
			goto bad;
		r->hi = strtol(p + 1, &end, 10);
		p = end;
	}
	snprintf(r->proto, sizeof r->proto, "tcp");
	if (*p == '/') {
		if (!strncmp(p + 1, "tcp", 3) || !strncmp(p + 1, "udp", 3)) {
			snprintf(r->proto, sizeof r->proto, "%.3s", p + 1);
			p += 4;
		} else {
			goto bad;
		}
	}
	if (*p && *p != ',' && *p != ' ')
		goto bad;
	if (r->lo < 1 || r->hi > 65535 || r->lo > r->hi) {
		if (err)
			snprintf(err, en, "puerto fuera de rango (1-65535)");
		return -1;
	}
	*s = p;
	return 1;
bad:
	if (err)
		snprintf(err, en, "formato de puerto no válido: usa PUERTO, PUERTO/tcp, PUERTO/udp o INICIO-FIN/tcp");
	return -1;
}

int rros_fw_ports_check(const char *list, char *err, size_t en)
{
	struct port_rule r;
	const char *s = list;
	int rc, n = 0;

	while ((rc = next_rule(&s, &r, err, en)) == 1)
		if (++n > 32) {
			if (err)
				snprintf(err, en, "demasiados puertos (máximo 32)");
			return -1;
		}
	return rc < 0 ? -1 : 0;
}

/* ---- cortafuegos (nftables) ---- */

void rros_render_nftables(FILE *f, const struct rros_secpol *p)
{
	struct port_rule r;
	const char *s = p->allow;

	fputs("#!/usr/sbin/nft -f\n"
	      "# Generado por rros-apply. No lo edites: usa `rros-security abrir PUERTO` o la clave\n"
	      "# security.firewall_allow. Solo toca la tabla de RROS; las de otros programas se respetan.\n\n"
	      "table inet rros\ndelete table inet rros\n\n", f);
	if (!p->firewall) {
		fputs("# Cortafuegos desactivado (security.firewall = false): no se filtra nada.\n", f);
		return;
	}
	fputs("table inet rros {\n"
	      "\tchain input {\n"
	      "\t\ttype filter hook input priority filter; policy drop;\n"
	      "\t\tct state established,related accept\n"
	      "\t\tct state invalid drop\n"
	      "\t\tiif \"lo\" accept\n"
	      "\t\t# IPv6 necesita ICMPv6 para funcionar (descubrimiento de vecinos)\n"
	      "\t\tmeta l4proto ipv6-icmp accept\n"
	      "\t\ticmp type { destination-unreachable, time-exceeded, parameter-problem } accept\n", f);
	if (p->ping)
		fputs("\t\ticmp type echo-request limit rate 10/second accept\n", f);
	fputs("\t\t# respuestas DHCP (IPv4 e IPv6)\n"
	      "\t\tudp sport 67 udp dport 68 accept\n"
	      "\t\tudp sport 547 udp dport 546 accept\n", f);
	if (p->printing)
		fputs("\t\t# services.printing: descubrir impresoras en la red (mDNS)\n"
		      "\t\tudp dport 5353 accept\n", f);
	if (p->ssh)
		fputs("\t\t# services.ssh\n\t\ttcp dport 22 accept\n", f);
	while (next_rule(&s, &r, NULL, 0) == 1) {
		if (r.lo == r.hi)
			fprintf(f, "\t\t%s dport %ld accept\n", r.proto, r.lo);
		else
			fprintf(f, "\t\t%s dport %ld-%ld accept\n", r.proto, r.lo, r.hi);
	}
	/* sin cadena forward: un escritorio no enruta, y una política drop ahí rompería Docker o libvirt */
	fputs("\t}\n}\n", f);
}

/* ---- kernel ---- */

const struct rros_sysctl rros_hardening[] = {
	{ "kernel.kptr_restrict", "2", "oculta direcciones del kernel a todos los usuarios" },
	{ "kernel.dmesg_restrict", "1", "solo root lee el registro del kernel" },
	{ "kernel.unprivileged_bpf_disabled", "2", "BPF solo para administradores" },
	{ "net.core.bpf_jit_harden", "2", "endurece el compilador JIT de BPF" },
	{ "kernel.yama.ptrace_scope", "1", "un programa solo puede depurar a sus hijos" },
	{ "dev.tty.ldisc_autoload", "0", "no carga disciplinas de línea a petición de usuarios" },
	{ "fs.protected_symlinks", "1", "enlaces simbólicos seguros en carpetas compartidas" },
	{ "fs.protected_hardlinks", "1", "enlaces duros seguros" },
	{ "fs.protected_fifos", "2", "FIFOs seguras en carpetas compartidas" },
	{ "fs.protected_regular", "2", "archivos seguros en carpetas compartidas" },
	{ "net.ipv4.tcp_syncookies", "1", "resiste inundaciones SYN" },
	{ "net.ipv4.conf.all.accept_redirects", "0", "ignora redirecciones ICMP" },
	{ "net.ipv4.conf.default.accept_redirects", "0", "ignora redirecciones ICMP (nuevas interfaces)" },
	{ "net.ipv6.conf.all.accept_redirects", "0", "ignora redirecciones ICMPv6" },
	{ "net.ipv4.conf.all.send_redirects", "0", "no envía redirecciones (no es un router)" },
	{ "net.ipv4.conf.all.accept_source_route", "0", "rechaza paquetes con ruta de origen" },
	{ "net.ipv4.icmp_echo_ignore_broadcasts", "1", "no responde a ping de difusión" },
};
const size_t rros_hardening_len = sizeof rros_hardening / sizeof rros_hardening[0];

void rros_render_hardening(FILE *f, const struct rros_secpol *p)
{
	fputs("# Generado por rros-apply (security.kernel_hardening)\n", f);
	if (!p->hardening) {
		fputs("# Desactivado: se usan los valores de Ubuntu. Los ya aplicados duran hasta reiniciar.\n", f);
		return;
	}
	for (size_t i = 0; i < rros_hardening_len; i++)
		fprintf(f, "# %s\n%s = %s\n", rros_hardening[i].why, rros_hardening[i].key, rros_hardening[i].value);
	/* sin volcados de memoria de programas con privilegios */
	if (!p->core_dumps)
		fputs("# security.core_dumps = false\nfs.suid_dumpable = 0\n", f);
}

void rros_render_modprobe(FILE *f, const struct rros_secpol *p)
{
	fputs("# Generado por rros-apply (security.usb_storage, security.camera)\n", f);
	if (!p->usb_storage)
		fputs("# memorias y discos USB bloqueados\ninstall usb-storage /bin/false\ninstall uas /bin/false\n", f);
	if (!p->camera)
		fputs("# cámaras web USB bloqueadas\ninstall uvcvideo /bin/false\n", f);
	if (p->usb_storage && p->camera)
		fputs("# sin bloqueos\n", f);
}

void rros_render_sudoers(FILE *f, const struct rros_secpol *p)
{
	fprintf(f, "# Generado por rros-apply (security.sudo_timeout_minutes)\n"
		"Defaults timestamp_timeout=%d\n", p->sudo_timeout);
}

void rros_render_faillock(FILE *f, const struct rros_secpol *p)
{
	fputs("# Generado por rros-apply (security.login_attempts). Lo usa pam_faillock.\n", f);
	if (p->login_attempts <= 0) {
		/* en Arch pam_faillock va siempre en system-auth (y sin archivo bloquearía a los 3 intentos) */
		fputs("# Sin límite de intentos.\ndeny = 0\n", f);
		return;
	}
	fprintf(f, "deny = %d\nfail_interval = 900\nunlock_time = 600\n", p->login_attempts);
}

void rros_render_autoupdates(FILE *f, const struct rros_secpol *p)
{
	fprintf(f, "// Generado por rros-apply (system.auto_updates)\n"
		"APT::Periodic::Update-Package-Lists \"%d\";\n"
		"APT::Periodic::Unattended-Upgrade \"%d\";\n", p->auto_updates, p->auto_updates);
}

int rros_is_arch(const char *root)
{
	char path[1024];

	snprintf(path, sizeof path, "%s/etc/arch-release", root ? root : "");
	return access(path, F_OK) == 0;
}

void rros_render_corelimit(FILE *f, const struct rros_secpol *p)
{
	fputs("# Generado por rros-apply (security.core_dumps)\n", f);
	if (!p->core_dumps)
		fputs("*\thard\tcore\t0\n", f);
}

/* ---- antivirus (ClamAV) ---- */

void rros_render_clamd(FILE *f, const struct rros_secpol *p)
{
	fputs("# Generado por rros-apply (security.av_*): el demonio de ClamAV y la protección en tiempo real.\n"
	      "LogSyslog yes\nLogTime yes\nExtendedDetectionInfo yes\n"
	      "LocalSocket /run/clamav/clamd.ctl\nFixStaleSocket yes\nUser clamav\n"
	      "DatabaseDirectory /var/lib/clamav\nDetectPUA yes\nAlertBrokenExecutables yes\n", f);
	fprintf(f, "MaxFileSize %dM\nMaxScanSize %dM\nStreamMaxLength %dM\n", p->av_max_mb, p->av_max_mb * 4, p->av_max_mb);
	/* tiempo real: clamonacc vigila las carpetas personales con fanotify y pregunta a clamd */
	fputs("OnAccessIncludePath /home\nOnAccessExcludeUname clamav\nOnAccessExcludeRootUID yes\n"
	      "OnAccessExtraScanning yes\nOnAccessPrevention no\n"
	      "VirusEvent /usr/libexec/rros-antivirus-evento\n", f);
}

void rros_render_avtimer(FILE *f, const struct rros_secpol *p)
{
	fputs("# Generado por rros-apply (security.av_schedule)\n[Timer]\nOnCalendar=\n", f);
	fprintf(f, "OnCalendar=%s\n", !strcmp(p->av_schedule, "diario") ? "daily" : "weekly");
}

void rros_render_avpolicy(FILE *f, const struct rros_secpol *p)
{
	fputs("# Generado por rros-apply: lo que hace rros-antivirus con lo que encuentra\n", f);
	fprintf(f, "ACCION=%s\nMAX_MB=%d\n", p->av_quarantine ? "cuarentena" : "avisar", p->av_max_mb);
}

void rros_render_pwquality(FILE *f, const struct rros_secpol *p)
{
	fputs("# Generado por rros-apply (security.password_*)\n", f);
	fprintf(f, "minlen = %d\n", p->pw_min > 0 ? p->pw_min : 1);
	if (p->pw_complex)
		fputs("minclass = 4\ndcredit = -1\nucredit = -1\nlcredit = -1\nocredit = -1\n", f);
	else
		fputs("minclass = 0\n", f);
	fputs("dictcheck = 1\nusercheck = 1\nenforce_for_root\n", f);
}

void rros_render_tmpmount(FILE *f, const struct rros_secpol *p)
{
	fputs("# Generado por rros-apply (security.tmp_noexec)\n[Mount]\n", f);
	fprintf(f, "Options=mode=1777,strictatime,nosuid,nodev%s,size=50%%%%,nr_inodes=1m\n", p->tmp_noexec ? ",noexec" : "");
}

/* ---- niveles ---- */

static const char *const lvl_basic[] = {
	"security.firewall=true", "security.firewall_ping=true", "security.kernel_hardening=false",
	"security.usb_storage=true", "security.camera=true", "security.login_attempts=0",
	"security.sudo_timeout_minutes=15", "security.core_dumps=true", "system.auto_updates=false", NULL
};
static const char *const lvl_standard[] = {
	"security.firewall=true", "security.firewall_ping=true", "security.kernel_hardening=true",
	"security.usb_storage=true", "security.camera=true", "security.login_attempts=10",
	"security.sudo_timeout_minutes=5", "security.core_dumps=false", "system.auto_updates=true", NULL
};
static const char *const lvl_strict[] = {
	"security.firewall=true", "security.firewall_ping=false", "security.kernel_hardening=true",
	"security.usb_storage=false", "security.camera=false", "security.login_attempts=5",
	"security.sudo_timeout_minutes=0", "security.core_dumps=false", "system.auto_updates=true",
	"services.ssh=false", NULL
};

const struct rros_seclevel rros_seclevels[] = {
	{ "basico", "cortafuegos y poco más", lvl_basic },
	{ "estandar", "recomendado: kernel endurecido, bloqueo tras 10 intentos, actualizaciones", lvl_standard },
	{ "estricto", "sin USB ni cámara, sin ping ni SSH, sudo pide siempre la contraseña", lvl_strict },
};
const size_t rros_seclevels_len = sizeof rros_seclevels / sizeof rros_seclevels[0];

const struct rros_seclevel *rros_seclevel_find(const char *name)
{
	for (size_t i = 0; i < rros_seclevels_len; i++)
		if (!strcmp(rros_seclevels[i].name, name))
			return &rros_seclevels[i];
	return NULL;
}
