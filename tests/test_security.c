/* Directivas de seguridad: lista de puertos, archivos generados y niveles. */
#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "rros_schema.h"
#include "rros_security.h"

typedef void (*renderer)(FILE *, const struct rros_secpol *);

static char *render(renderer fn, const struct rros_secpol *p)
{
	char *text = NULL;
	size_t len = 0;
	FILE *m = open_memstream(&text, &len);

	fn(m, p);
	fclose(m);
	return text;
}

static void base(struct rros_secpol *p)
{
	memset(p, 0, sizeof *p);
	p->firewall = p->ping = p->hardening = p->usb_storage = p->camera = 1;
	p->login_attempts = 10;
	p->sudo_timeout = 5;
}

int main(void)
{
	struct rros_secpol p;
	char err[200], *s;

	/* ---- lista de puertos ---- */
	CHECK(rros_fw_ports_check("", err, sizeof err) == 0);
	CHECK(rros_fw_ports_check("8080", err, sizeof err) == 0);
	CHECK(rros_fw_ports_check("8080/tcp,5000-5010/udp, 53", err, sizeof err) == 0);
	CHECK(rros_fw_ports_check("0", err, sizeof err) != 0);
	CHECK(rros_fw_ports_check("70000", err, sizeof err) != 0);
	CHECK(rros_fw_ports_check("10-5", err, sizeof err) != 0);
	CHECK(rros_fw_ports_check("22/icmp", err, sizeof err) != 0);
	CHECK(rros_fw_ports_check("22;drop", err, sizeof err) != 0);
	CHECK(rros_fw_ports_check("22 accept\nflush ruleset", err, sizeof err) != 0);
	CHECK(rros_fw_ports_check("abc", err, sizeof err) != 0);
	{
		char many[400] = "";

		for (int i = 1; i <= 33; i++)
			snprintf(many + strlen(many), sizeof many - strlen(many), "%s%d", i > 1 ? "," : "", 1000 + i);
		CHECK(rros_fw_ports_check(many, err, sizeof err) != 0);
	}

	/* el esquema usa la misma validación */
	{
		const struct rros_key *k = rros_schema_find("security.firewall_allow");

		CHECK(k != NULL);
		CHECK(k && rros_schema_check(k, "8080/tcp", 1, err, sizeof err) == 0);
		CHECK(k && rros_schema_check(k, "8080; flush ruleset", 1, err, sizeof err) != 0);
		CHECK(rros_schema_find("services.firewall") == NULL);
	}

	/* ---- cortafuegos ---- */
	base(&p);
	s = render(rros_render_nftables, &p);
	CHECK(strstr(s, "table inet rros\ndelete table inet rros\n") != NULL);   /* idempotente */
	CHECK(strstr(s, "policy drop") != NULL);
	CHECK(strstr(s, "ct state established,related accept") != NULL);
	CHECK(strstr(s, "echo-request") != NULL);
	CHECK(strstr(s, "dport 22") == NULL);
	CHECK(strstr(s, "chain forward") == NULL);   /* no rompe Docker ni libvirt */
	CHECK(strstr(s, "flush ruleset") == NULL);   /* no borra tablas de otros programas */
	free(s);

	p.ping = 0;
	p.ssh = 1;
	p.printing = 1;
	snprintf(p.allow, sizeof p.allow, "8080/tcp,5000-5010/udp,53");
	s = render(rros_render_nftables, &p);
	CHECK(strstr(s, "echo-request") == NULL);
	CHECK(strstr(s, "tcp dport 22 accept") != NULL);
	CHECK(strstr(s, "udp dport 5353 accept") != NULL);
	CHECK(strstr(s, "tcp dport 8080 accept") != NULL);
	CHECK(strstr(s, "udp dport 5000-5010 accept") != NULL);
	CHECK(strstr(s, "tcp dport 53 accept") != NULL);
	free(s);

	p.firewall = 0;
	s = render(rros_render_nftables, &p);
	CHECK(strstr(s, "policy drop") == NULL);
	CHECK(strstr(s, "delete table inet rros") != NULL);   /* desactivar quita nuestras reglas */
	free(s);

	/* ---- kernel ---- */
	base(&p);
	s = render(rros_render_hardening, &p);
	for (size_t i = 0; i < rros_hardening_len; i++) {
		char line[128];

		snprintf(line, sizeof line, "%s = %s\n", rros_hardening[i].key, rros_hardening[i].value);
		CHECK(strstr(s, line) != NULL);
	}
	CHECK(strstr(s, "fs.suid_dumpable = 0") != NULL);
	CHECK(strstr(s, "kexec_load_disabled") == NULL);   /* nada irreversible hasta reiniciar */
	free(s);
	p.hardening = 0;
	s = render(rros_render_hardening, &p);
	CHECK(strstr(s, " = ") == NULL);
	free(s);

	/* ---- USB y cámara ---- */
	base(&p);
	s = render(rros_render_modprobe, &p);
	CHECK(strstr(s, "install") == NULL);
	free(s);
	p.usb_storage = 0;
	p.camera = 0;
	s = render(rros_render_modprobe, &p);
	CHECK(strstr(s, "install usb-storage /bin/false") != NULL);
	CHECK(strstr(s, "install uas /bin/false") != NULL);
	CHECK(strstr(s, "install uvcvideo /bin/false") != NULL);
	free(s);

	/* ---- sudo, intentos, actualizaciones, volcados ---- */
	base(&p);
	s = render(rros_render_sudoers, &p);
	CHECK(strstr(s, "Defaults timestamp_timeout=5\n") != NULL);
	free(s);
	s = render(rros_render_faillock, &p);
	CHECK(strstr(s, "deny = 10\n") != NULL && strstr(s, "unlock_time = 600") != NULL);
	free(s);
	p.login_attempts = 0;
	s = render(rros_render_faillock, &p);
	CHECK(strstr(s, "deny = 0\n") != NULL);
	free(s);
	p.auto_updates = 1;
	s = render(rros_render_autoupdates, &p);
	CHECK(strstr(s, "APT::Periodic::Unattended-Upgrade \"1\";") != NULL);
	free(s);
	s = render(rros_render_corelimit, &p);
	CHECK(strstr(s, "hard\tcore\t0") != NULL);
	free(s);
	p.core_dumps = 1;
	s = render(rros_render_corelimit, &p);
	CHECK(strstr(s, "core\t0") == NULL);
	free(s);

	/* ---- niveles: cada clave existe y cada valor pasa el esquema ---- */
	CHECK(rros_seclevels_len == 3);
	CHECK(rros_seclevel_find("estandar") != NULL);
	CHECK(rros_seclevel_find("paranoico") == NULL);
	for (size_t i = 0; i < rros_seclevels_len; i++)
		for (const char *const *kv = rros_seclevels[i].kv; *kv; kv++) {
			char key[96];
			const char *eq = strchr(*kv, '=');
			const struct rros_key *k;

			CHECK(eq != NULL);
			if (!eq)
				continue;
			snprintf(key, sizeof key, "%.*s", (int)(eq - *kv), *kv);
			k = rros_schema_find(key);
			CHECK(k != NULL);
			if (k)
				CHECK(rros_schema_check(k, eq + 1, 0, err, sizeof err) == 0);
		}
	/* estricto es más estricto que estándar en lo que importa */
	{
		const struct rros_seclevel *st = rros_seclevel_find("estricto");
		int usb_off = 0, cam_off = 0;

		for (const char *const *kv = st->kv; *kv; kv++) {
			usb_off |= !strcmp(*kv, "security.usb_storage=false");
			cam_off |= !strcmp(*kv, "security.camera=false");
		}
		CHECK(usb_off && cam_off);
	}
	DONE();
}
