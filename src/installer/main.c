/* rros-installer: cálculos del instalador. No escribe en ningún disco (eso lo hace rros-install).
 *   plan    esquema de particiones (borrando el disco o junto a otro sistema)
 *   disks   discos donde se puede instalar, con su espacio libre y los sistemas que tienen
 * RROS_ROOT antepone un directorio a /sys, /proc y /run (pruebas). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "plan.h"
#include "rros_devices.h"
#include "rros_disks.h"
#include "rros_hw.h"
#include "rros_profile.h"

#define MAX_DISKS 16

static struct rros_disk disks_g[MAX_DISKS];

static void usage(void)
{
	fputs("uso: rros-installer plan (--disk-gb N | --disk DISCO [--alongside]) [--encrypt]\n"
	      "                         [--uefi|--bios] [-p PERFIL] [--sfdisk|--table]\n"
	      "       rros-installer disks [--json] [--uefi|--bios] [-p PERFIL]\n"
	      "  plan calcula el esquema de particiones y disks lista los discos; ninguno modifica nada.\n"
	      "  --alongside usa el mayor espacio libre del disco (GPT) sin tocar lo que ya hay.\n", stderr);
}

static const char *root_dir(void)
{
	const char *r = getenv("RROS_ROOT");

	return r ? r : "";
}

/* espacio que necesita el perfil: su presupuesto de disco más 2 GB de margen */
static int profile_need(char *profile, size_t pn, long *need_mb)
{
	struct rros_conf prof;

	if (!*profile)
		rros_read_active_profile(profile, pn);
	rros_conf_init(&prof);
	if (rros_load_profile(&prof, profile, NULL) != 0) {
		fprintf(stderr, "rros-installer: perfil '%s' no encontrado\n", profile);
		rros_conf_free(&prof);
		return -1;
	}
	*need_mb = (rros_conf_get_long(&prof, "profile.max_disk_gb", 20) + 2) * 1024;
	rros_conf_free(&prof);
	return 0;
}

static int detect_uefi(void)
{
	struct rros_hw hw;

	return rros_hw_detect(&hw, NULL) == 0 ? hw.uefi : 1;
}

static const struct rros_disk *find_disk(int n, const char *dev)
{
	const char *name = !strncmp(dev, "/dev/", 5) ? dev + 5 : dev;

	for (int i = 0; i < n; i++)
		if (!strcmp(disks_g[i].name, name))
			return &disks_g[i];
	return NULL;
}

/* Rellena la petición para un disco real; 0 si el disco sirve para ese modo (si no, err). */
static int req_for_disk(struct rros_plan_req *r, const struct rros_disk *d, int alongside, char *err, size_t en)
{
	r->disk_mb = (long)(d->size / RROS_MIB);
	r->alongside = alongside;
	if (*d->in_use) {
		snprintf(err, en, "%s", d->in_use);
		return -1;
	}
	if (!alongside)
		return 0;
	if (strcmp(d->pttype, "gpt")) {
		snprintf(err, en, "%s", !*d->pttype ? "el disco está vacío: elige «borrar todo»" :
			 "la tabla de particiones es MBR: junto a otro sistema solo se instala en discos GPT (UEFI)");
		return -1;
	}
	r->free_start_mb = (long)(d->free_start / RROS_MIB);
	r->free_mb = (long)(d->free_size / RROS_MIB);
	if (d->esp >= 0) {
		r->esp_start_mb = (long)(d->parts[d->esp].start / RROS_MIB);
		r->esp_mb = (long)(d->parts[d->esp].size / RROS_MIB);
	}
	r->have_bios_boot = d->bios_boot;
	return 0;
}

static int check(const struct rros_plan_req *base, const struct rros_disk *d, int alongside, int encrypt,
		 char *err, size_t en)
{
	struct rros_plan_req r = *base;
	struct rros_plan p;

	*err = '\0';
	r.encrypt = encrypt;
	if (req_for_disk(&r, d, alongside, err, en))
		return -1;
	return rros_plan_build(&p, &r, err, en);
}

static void json_str(const char *s)
{
	putchar('"');
	for (; *s; s++) {
		unsigned char c = (unsigned char)*s;

		if (c == '"' || c == '\\')
			printf("\\%c", c);
		else if (c < 0x20)
			printf("\\u%04x", c);
		else
			putchar(c);
	}
	putchar('"');
}

static const char *part_role(const struct rros_dpart *p)
{
	return p->esp ? "esp" : p->bios_boot ? "bios_boot" : p->windows ? "windows" : p->linux_os ? "linux" : "";
}

static int cmd_disks(int argc, char **argv)
{
	int json = 0, uefi = -1, n;
	char profile[64] = "", err[256], sz[24], fr[24], sys[64];
	struct rros_plan_req base = { 0 };

	for (int i = 2; i < argc; i++) {
		if (!strcmp(argv[i], "--json"))
			json = 1;
		else if (!strcmp(argv[i], "--uefi"))
			uefi = 1;
		else if (!strcmp(argv[i], "--bios"))
			uefi = 0;
		else if (!strcmp(argv[i], "-p") && i + 1 < argc)
			snprintf(profile, sizeof profile, "%s", argv[++i]);
		else {
			usage();
			return 2;
		}
	}
	if (uefi < 0)
		uefi = detect_uefi();
	if (profile_need(profile, sizeof profile, &base.min_mb))
		return 1;
	base.profile = profile;
	base.uefi = uefi;
	n = rros_disks_scan(disks_g, MAX_DISKS, root_dir());

	if (json) {
		printf("{\"firmware\":\"%s\",\"profile\":", uefi ? "uefi" : "bios");
		json_str(profile);
		printf(",\"min_mb\":%ld,\"disks\":[", base.min_mb);
	} else {
		printf("firmware=%s perfil=%s necesita=%.1f GB\n", uefi ? "uefi" : "bios", profile, base.min_mb / 1024.0);
	}
	for (int i = 0; i < n; i++) {
		const struct rros_disk *d = &disks_g[i];
		/* qué modos admite: "" = sí; si no, por qué */
		static const int modes[3][2] = { { 0, 0 }, { 1, 0 }, { 1, 1 } };
		static const char *const names[3] = { "wipe", "alongside", "alongside_crypt" };

		rros_human_size(d->size, sz, sizeof sz);
		rros_human_size(d->free_size, fr, sizeof fr);
		rros_disk_systems(d, sys, sizeof sys);
		if (!json) {
			printf("/dev/%-10s %9s  %-24.24s %-10s libre %-9s %s%s%s\n", d->name, sz, d->model, *d->pttype ? d->pttype : "vacío",
			       d->free_size ? fr : "-", sys, *sys && *d->in_use ? " · " : "", d->in_use);
			continue;
		}
		printf("%s{\"name\":", i ? "," : "");
		json_str(d->name);
		printf(",\"path\":\"/dev/");
		fputs(d->name, stdout);
		printf("\",\"model\":");
		json_str(d->model);
		printf(",\"kind\":");
		json_str(d->kind);
		printf(",\"size\":%llu,\"size_h\":", d->size);
		json_str(sz);
		printf(",\"pttype\":");
		json_str(d->pttype);
		printf(",\"in_use\":");
		json_str(d->in_use);
		printf(",\"systems\":");
		json_str(sys);
		printf(",\"windows\":%s,\"bitlocker\":%s,\"esp\":", d->windows ? "true" : "false", d->bitlocker ? "true" : "false");
		json_str(d->esp >= 0 ? d->parts[d->esp].name : "");
		printf(",\"free_mb\":%llu,\"free_h\":", d->free_size / RROS_MIB);
		json_str(d->free_size ? fr : "");
		for (int m = 0; m < 3; m++) {
			check(&base, d, modes[m][0], modes[m][1], err, sizeof err);
			printf(",\"%s\":", names[m]);
			json_str(err);
		}
		printf(",\"parts\":[");
		for (int k = 0; k < d->nparts; k++) {
			const struct rros_dpart *p = &d->parts[k];

			rros_human_size(p->size, sz, sizeof sz);
			printf("%s{\"name\":", k ? "," : "");
			json_str(p->name);
			printf(",\"start_mb\":%llu,\"size_mb\":%llu,\"size_h\":", p->start / RROS_MIB, p->size / RROS_MIB);
			json_str(sz);
			printf(",\"fstype\":");
			json_str(p->fstype);
			printf(",\"label\":");
			json_str(p->label);
			printf(",\"mount\":");
			json_str(p->mount);
			printf(",\"role\":\"%s\"}", part_role(p));
		}
		printf("]}");
	}
	if (json)
		puts("]}");
	return 0;
}

static int cmd_plan(int argc, char **argv)
{
	long disk_gb = 0;
	int uefi = -1, out = 0, alongside = 0, encrypt = 0;   /* out: 0 texto, 1 sfdisk, 2 tabla */
	const char *disk = NULL;
	char profile[64] = "", err[256], script[1024];
	struct rros_plan_req req = { 0 };
	struct rros_plan plan;

	for (int i = 2; i < argc; i++) {
		if (!strcmp(argv[i], "--disk-gb") && i + 1 < argc)
			disk_gb = atol(argv[++i]);
		else if (!strcmp(argv[i], "--disk") && i + 1 < argc)
			disk = argv[++i];
		else if (!strcmp(argv[i], "--alongside"))
			alongside = 1;
		else if (!strcmp(argv[i], "--encrypt"))
			encrypt = 1;
		else if (!strcmp(argv[i], "--uefi"))
			uefi = 1;
		else if (!strcmp(argv[i], "--bios"))
			uefi = 0;
		else if (!strcmp(argv[i], "--sfdisk"))
			out = 1;
		else if (!strcmp(argv[i], "--table"))
			out = 2;
		else if (!strcmp(argv[i], "-p") && i + 1 < argc)
			snprintf(profile, sizeof profile, "%s", argv[++i]);
		else {
			usage();
			return 2;
		}
	}
	if ((disk_gb <= 0) == !disk || (alongside && !disk)) {
		usage();
		return 2;
	}
	if (uefi < 0)
		uefi = detect_uefi();
	if (profile_need(profile, sizeof profile, &req.min_mb))
		return 1;
	req.profile = profile;
	req.uefi = uefi;
	req.encrypt = encrypt;
	if (disk) {
		int n = rros_disks_scan(disks_g, MAX_DISKS, root_dir());
		const struct rros_disk *d = find_disk(n, disk);

		if (!d) {
			fprintf(stderr, "rros-installer: %s no es un disco\n", disk);
			return 1;
		}
		if (req_for_disk(&req, d, alongside, err, sizeof err)) {
			fprintf(stderr, "rros-installer: %s\n", err);
			return 1;
		}
	} else {
		req.disk_mb = disk_gb * 1024;
	}
	if (rros_plan_build(&plan, &req, err, sizeof err)) {
		fprintf(stderr, "rros-installer: %s\n", err);
		return 1;
	}
	if (out) {
		(out == 1 ? rros_plan_sfdisk : rros_plan_table)(&plan, script, sizeof script);
		fputs(script, stdout);
		return 0;
	}
	printf("perfil=%s firmware=%s modo=%s%s\n", profile, uefi ? "uefi" : "bios",
	       alongside ? "junto a lo que hay" : "borrar todo", encrypt ? " cifrado" : "");
	for (int i = 0; i < plan.n; i++) {
		const struct rros_part *x = &plan.parts[i];
		char size[24];

		rros_human_size((unsigned long long)x->size_mb * RROS_MIB, size, sizeof size);
		printf("%-10s %-6s %10s  %-10s%s%s\n", x->label, x->fs, size, x->mount,
		       x->reuse ? " (la que ya hay)" : "", x->crypt ? " (cifrada con LUKS2)" : "");
	}
	puts("swap=zram (sin partición)");
	return 0;
}

int main(int argc, char **argv)
{
	if (argc >= 2 && !strcmp(argv[1], "plan"))
		return cmd_plan(argc, argv);
	if (argc >= 2 && !strcmp(argv[1], "disks"))
		return cmd_disks(argc, argv);
	usage();
	return 2;
}
