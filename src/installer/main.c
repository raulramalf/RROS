/* rros-installer: por ahora solo `plan` (simulación segura, no escribe en disco). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "plan.h"
#include "rros_hw.h"
#include "rros_profile.h"

static void usage(void)
{
	fputs("uso: rros-installer plan --disk-gb N [--uefi|--bios] [-p PERFIL] [--sfdisk]\n"
	      "  Calcula el esquema de particiones. No modifica ningún disco.\n", stderr);
}

int main(int argc, char **argv)
{
	long disk_gb = 0;
	int uefi = -1, sfdisk = 0;
	char profile[64] = "";
	struct rros_conf prof;
	struct rros_plan plan;
	char err[256], script[1024];
	long headroom_gb = 2, need_mb;

	if (argc < 2 || strcmp(argv[1], "plan") != 0) {
		usage();
		return 2;
	}
	for (int i = 2; i < argc; i++) {
		if (!strcmp(argv[i], "--disk-gb") && i + 1 < argc)
			disk_gb = atol(argv[++i]);
		else if (!strcmp(argv[i], "--uefi"))
			uefi = 1;
		else if (!strcmp(argv[i], "--bios"))
			uefi = 0;
		else if (!strcmp(argv[i], "--sfdisk"))
			sfdisk = 1;
		else if (!strcmp(argv[i], "-p") && i + 1 < argc)
			snprintf(profile, sizeof profile, "%s", argv[++i]);
		else {
			usage();
			return 2;
		}
	}
	if (disk_gb <= 0) {
		usage();
		return 2;
	}
	if (uefi < 0) {
		struct rros_hw hw;

		uefi = rros_hw_detect(&hw, NULL) == 0 ? hw.uefi : 1;
	}
	if (!*profile)
		rros_read_active_profile(profile, sizeof profile);

	rros_conf_init(&prof);
	if (rros_load_profile(&prof, profile, NULL) != 0) {
		fprintf(stderr, "rros-installer: perfil '%s' no encontrado\n", profile);
		return 1;
	}
	need_mb = (rros_conf_get_long(&prof, "profile.max_disk_gb", 20) + headroom_gb) * 1024;
	rros_conf_free(&prof);

	if (rros_plan_make(&plan, profile, disk_gb * 1024, uefi, need_mb, err, sizeof err)) {
		fprintf(stderr, "rros-installer: %s\n", err);
		return 1;
	}
	if (sfdisk) {
		rros_plan_sfdisk(&plan, script, sizeof script);
		fputs(script, stdout);
		return 0;
	}
	printf("perfil=%s firmware=%s\n", profile, uefi ? "uefi" : "bios");
	for (int i = 0; i < plan.n; i++) {
		const struct rros_part *x = &plan.parts[i];

		if (x->size_mb)
			printf("%-10s %-6s %6ld MiB  %s\n", x->label, x->fs, x->size_mb, x->mount);
		else
			printf("%-10s %-6s %10s  %s\n", x->label, x->fs, "resto", x->mount);
	}
	puts("swap=zram (sin partición)");
	return 0;
}
