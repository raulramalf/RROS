/* rros-hwdetect: detecta el hardware y propone un perfil RROS. */
#include <stdio.h>
#include <string.h>

#include "rros_hw.h"
#include "rros_profile.h"

static void json_str(const char *s)
{
	putchar('"');
	for (; *s; s++) {
		if (*s == '"' || *s == '\\')
			printf("\\%c", *s);
		else if ((unsigned char)*s < 0x20)
			printf("\\u%04x", (unsigned char)*s);
		else
			putchar(*s);
	}
	putchar('"');
}

static void usage(void)
{
	fputs("uso: rros-hwdetect [--json | --profile | --apply] [--root DIR]\n"
	      "  (sin opciones)  imprime clave=valor\n"
	      "  --profile       imprime solo el perfil recomendado\n"
	      "  --apply         guarda el perfil en el archivo de perfil activo\n", stderr);
}

int main(int argc, char **argv)
{
	enum { KV, JSON, PROFILE, APPLY } mode = KV;
	const char *root = NULL;
	struct rros_hw hw;
	const char *prof;

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--json"))
			mode = JSON;
		else if (!strcmp(argv[i], "--profile"))
			mode = PROFILE;
		else if (!strcmp(argv[i], "--apply"))
			mode = APPLY;
		else if (!strcmp(argv[i], "--root") && i + 1 < argc)
			root = argv[++i];
		else {
			usage();
			return 2;
		}
	}
	if (rros_hw_detect(&hw, root) != 0) {
		fputs("rros-hwdetect: no se pudo leer /proc/meminfo\n", stderr);
		return 1;
	}
	prof = rros_hw_pick_profile(&hw);

	switch (mode) {
	case PROFILE:
		puts(prof);
		break;
	case APPLY:
		if (rros_write_active_profile(prof) != 0) {
			fprintf(stderr, "rros-hwdetect: no se pudo escribir %s\n", rros_profile_file());
			return 1;
		}
		printf("perfil %s guardado en %s\n", prof, rros_profile_file());
		break;
	case JSON:
		printf("{\"ram_mb\":%ld,\"cpus\":%d,\"arch\":", hw.ram_mb, hw.cpus);
		json_str(hw.arch);
		printf(",\"cpu_model\":");
		json_str(hw.cpu_model);
		printf(",\"gpu\":\"%s\",\"uefi\":%s,\"profile\":\"%s\"}\n",
		       rros_gpu_name(hw.gpu), hw.uefi ? "true" : "false", prof);
		break;
	default:
		printf("ram_mb=%ld\ncpus=%d\narch=%s\ncpu_model=%s\ngpu=%s\nuefi=%d\nprofile=%s\n",
		       hw.ram_mb, hw.cpus, hw.arch, hw.cpu_model,
		       rros_gpu_name(hw.gpu), hw.uefi, prof);
	}
	return 0;
}
