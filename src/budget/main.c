/* rros-budget: comprueba el presupuesto de RAM/disco de un perfil (para CI). */
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "rros_hw.h"
#include "rros_profile.h"

static long long total_bytes;

static int visit(const char *path, const struct stat *st, int type, struct FTW *w)
{
	(void)path; (void)type; (void)w;
	total_bytes += (long long)st->st_blocks * 512;
	return 0;
}

static void usage(void)
{
	fputs("uso: rros-budget ram [-p PERFIL]\n"
	      "     rros-budget disk DIR [-p PERFIL]\n"
	      "  Sale con 0 si cumple el presupuesto, 1 si lo supera, 2 en error de uso.\n", stderr);
}

int main(int argc, char **argv)
{
	char profile[64] = "";
	const char *dir = NULL;
	struct rros_conf prof;
	const char *root = getenv("RROS_ROOT");
	long used, limit;
	int ram;

	if (argc < 2) {
		usage();
		return 2;
	}
	if (!strcmp(argv[1], "ram"))
		ram = 1;
	else if (!strcmp(argv[1], "disk") && argc >= 3)
		ram = 0;
	else {
		usage();
		return 2;
	}
	for (int i = 2; i < argc; i++) {
		if (!strcmp(argv[i], "-p") && i + 1 < argc)
			snprintf(profile, sizeof profile, "%s", argv[++i]);
		else if (!ram && !dir)
			dir = argv[i];
		else {
			usage();
			return 2;
		}
	}
	if (!*profile)
		rros_read_active_profile(profile, sizeof profile);
	rros_conf_init(&prof);
	if (rros_load_profile(&prof, profile, NULL) != 0) {
		fprintf(stderr, "rros-budget: perfil '%s' no encontrado\n", profile);
		return 2;
	}
	if (ram) {
		used = rros_hw_ram_used_mb(root);
		limit = rros_conf_get_long(&prof, "profile.max_idle_ram_mb", 0);
		if (used < 0) {
			fputs("rros-budget: no se pudo leer la memoria\n", stderr);
			return 2;
		}
	} else {
		if (nftw(dir, visit, 32, FTW_PHYS | FTW_MOUNT) != 0) {
			perror("rros-budget");
			return 2;
		}
		used = (long)(total_bytes / (1024 * 1024));
		limit = rros_conf_get_long(&prof, "profile.max_disk_gb", 0) * 1024;
	}
	rros_conf_free(&prof);
	printf("profile=%s resource=%s used_mb=%ld limit_mb=%ld status=%s\n", profile,
	       ram ? "ram" : "disk", used, limit, used <= limit ? "ok" : "over");
	return used <= limit ? 0 : 1;
}
