/* rros-settings: ajustes de RROS. Abre la aplicación RROS Ajustes, la versión de terminal o responde a la API. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "settings.h"

static void usage(FILE *out)
{
	fputs("uso: rros-settings [COMANDO]\n"
	      "  open [PÁGINA]       abre RROS Ajustes (la aplicación; sin pantalla, la versión de terminal)\n"
	      "PÁGINA: general, apariencia, escritorio, red, bluetooth, sonido, pantalla, energia, raton, teclado,\n"
	      "        tienda, seguridad, dispositivos, actualizaciones, accesibilidad, avanzado, instalar, bienvenida\n"
	      "  tui [--no-apply]    interfaz de terminal (ideal en Lite o por SSH)\n"
	      "  state               estado completo en JSON (para scripts)\n"
	      "  api MÉTODO RUTA [CUERPO]   lo que usa la aplicación (JSON por la salida estándar)\n"
	      "\n"
	      "Cada cambio se valida igual que con `rros-config set` y se guarda al instante.\n", out);
}

static int page_ok(const char *p)
{
	size_t n = strlen(p);

	if (n == 0 || n > 31)
		return 0;
	for (; *p; p++)
		if (!((*p >= 'a' && *p <= 'z') || *p == ':' || *p == '-'))
			return 0;
	return 1;
}

static int open_best(const char *page)
{
	int have_display = getenv("WAYLAND_DISPLAY") || getenv("DISPLAY");

	if (have_display) {
		char *args[] = { "rros-ajustes", page ? "--pagina" : NULL, (char *)page, NULL };

		if (spawn_detached("rros-ajustes", args) == 0)
			return 0;
	}
	if (isatty(STDIN_FILENO))
		return run_tui(1);
	{
		char *args[] = { "konsole", "-e", "rros-settings", "tui", NULL };

		if (run_tool("konsole", args, 0) == 127) {
			fputs("rros-settings: no está RROS Ajustes ni hay terminal; usa `rros-settings tui`\n", stderr);
			return 1;
		}
		return 0;
	}
}

int main(int argc, char **argv)
{
	const char *cmd = argc > 1 ? argv[1] : "open";

	tool_dir_from_argv0(argv[0]);
	if (!strcmp(cmd, "--help") || !strcmp(cmd, "help") || !strcmp(cmd, "-h")) {
		usage(stdout);
		return 0;
	}
	if (!strcmp(cmd, "open")) {
		if (argc > 2 && !page_ok(argv[2])) {
			fprintf(stderr, "rros-settings: página no válida '%s'\n", argv[2]);
			return 2;
		}
		return open_best(argc > 2 ? argv[2] : NULL);
	}
	if (!strcmp(cmd, "api")) {
		if (argc < 4 || argc > 5) {
			usage(stderr);
			return 2;
		}
		return run_api(argv[2], argv[3], argc > 4 ? argv[4] : NULL);
	}
	if (!strcmp(cmd, "state")) {
		struct model m;
		struct sbuf b = { 0 };

		if (model_load(&m) != 0) {
			fputs("rros-settings: no se pudo cargar el perfil\n", stderr);
			return 1;
		}
		if (model_state_json(&m, &b) == 0)
			puts(b.p);
		sb_free(&b);
		model_free(&m);
		return 0;
	}
	if (!strcmp(cmd, "tui")) {
		int apply = 1;

		for (int i = 2; i < argc; i++) {
			if (!strcmp(argv[i], "--no-apply"))
				apply = 0;
			else {
				usage(stderr);
				return 2;
			}
		}
		return run_tui(apply);
	}
	usage(stderr);
	return 2;
}
