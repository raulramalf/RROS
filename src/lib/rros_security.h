/* Directivas de seguridad: de la configuración (claves security.*) a archivos del sistema.
 * Lo usan rros-apply (escribe los archivos), rros-security (audita y aplica niveles) y los tests. */
#ifndef RROS_SECURITY_H
#define RROS_SECURITY_H

#include <stddef.h>
#include <stdio.h>
#include "rros_conf.h"

struct rros_secpol {
	int firewall;          /* security.firewall */
	int ping;              /* security.firewall_ping */
	char allow[256];       /* security.firewall_allow: "8080/tcp,5000-5010/udp" */
	int hardening;         /* security.kernel_hardening */
	int usb_storage;       /* security.usb_storage (1 = permitido) */
	int camera;            /* security.camera (1 = permitida) */
	int login_attempts;    /* security.login_attempts (0 = sin límite) */
	int sudo_timeout;      /* security.sudo_timeout_minutes */
	int core_dumps;        /* security.core_dumps */
	int auto_updates;      /* system.auto_updates */
	int ssh;               /* services.ssh: abre el 22/tcp */
	int printing;          /* services.printing: abre mDNS para encontrar impresoras */
	int av_realtime;       /* security.av_realtime: clamd + clamonacc */
	char av_schedule[12];  /* security.av_schedule: off, diario, semanal */
	int av_quarantine;     /* security.av_action == cuarentena */
	int av_updates;        /* security.av_updates: freshclam */
	int av_max_mb;         /* security.av_max_file_mb */
	int pw_min;            /* security.password_min_length */
	int pw_complex;        /* security.password_complex */
	int usb_guard;         /* security.usb_guard */
	int tmp_noexec;        /* security.tmp_noexec */
};

void rros_secpol_from_conf(struct rros_secpol *p, const struct rros_conf *c);

/* Lista de puertos del cortafuegos: "PUERTO[-PUERTO][/tcp|/udp]" separados por comas.
 * 0 si es válida (o vacía); -1 y mensaje en err si no. */
int rros_fw_ports_check(const char *list, char *err, size_t en);

/* Endurecimiento del kernel: sysctl que aplica security.kernel_hardening (y que audita rros-security). */
struct rros_sysctl { const char *key, *value, *why; };
extern const struct rros_sysctl rros_hardening[];
extern const size_t rros_hardening_len;

/* Generadores: escriben el contenido completo de cada archivo. */
void rros_render_nftables(FILE *f, const struct rros_secpol *p);   /* /etc/nftables.conf */
void rros_render_hardening(FILE *f, const struct rros_secpol *p);  /* /etc/sysctl.d/80-rros-security.conf */
void rros_render_modprobe(FILE *f, const struct rros_secpol *p);   /* /etc/modprobe.d/rros-security.conf */
void rros_render_sudoers(FILE *f, const struct rros_secpol *p);    /* /etc/sudoers.d/rros (0440) */
void rros_render_faillock(FILE *f, const struct rros_secpol *p);   /* /etc/security/faillock.conf */
void rros_render_autoupdates(FILE *f, const struct rros_secpol *p);/* /etc/apt/apt.conf.d/20auto-upgrades (Ubuntu) */
void rros_render_corelimit(FILE *f, const struct rros_secpol *p);  /* /etc/security/limits.d/90-rros-core.conf */
void rros_render_clamd(FILE *f, const struct rros_secpol *p);      /* /etc/clamav/clamd.conf */
void rros_render_avtimer(FILE *f, const struct rros_secpol *p);    /* /etc/systemd/system/rros-antivirus.timer.d/rros.conf */
void rros_render_avpolicy(FILE *f, const struct rros_secpol *p);   /* /etc/rros/antivirus.conf (lo lee rros-antivirus) */
void rros_render_pwquality(FILE *f, const struct rros_secpol *p);  /* /etc/security/pwquality.conf.d/rros.conf */
void rros_render_tmpmount(FILE *f, const struct rros_secpol *p);   /* /etc/systemd/system/tmp.mount.d/rros.conf */

/* 1 si el sistema bajo root (""= /) es Arch: en Arch faillock va siempre en /etc/pam.d/system-auth, las
 * actualizaciones se descargan con rros-updates.timer (no hay unattended-upgrades) y ssh es sshd.service. */
int rros_is_arch(const char *root);

/* Niveles predefinidos: pares clave=valor que `rros-security level` escribe en la capa del sistema. */
struct rros_seclevel { const char *name, *desc; const char *const *kv; };
extern const struct rros_seclevel rros_seclevels[];
extern const size_t rros_seclevels_len;
const struct rros_seclevel *rros_seclevel_find(const char *name);

#endif
