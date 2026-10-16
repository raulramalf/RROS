/* rros-settings: ajustes de RROS. Interfaz gráfica web local y versión de terminal. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "settings.h"

static void usage(FILE *out)
{
	fputs("uso: rros-settings [COMANDO]\n"
	      "  open                abre la interfaz que mejor encaje (por defecto)\n"
	      "  web [--port N] [--no-open] [--timeout S]   interfaz gráfica en tu navegador\n"
	      "  tui [--no-apply]    interfaz de terminal (ideal en Lite o por SSH)\n"
	      "  state               estado completo en JSON (para scripts)\n"
	      "\n"
	      "La interfaz web solo escucha en 127.0.0.1 y exige un token aleatorio.\n"
	      "Cada cambio se valida igual que con `rros-config set` y se guarda al instante.\n", out);
}

static int open_best(void)
{
	int have_display = getenv("WAYLAND_DISPLAY") || getenv("DISPLAY");

	if (have_display && web_browser_available())
		return run_web(0, 1, NULL, 3600);
	if (isatty(STDIN_FILENO))
		return run_tui(1);
	{
		char *args[] = { "foot", "-T", "RROS Ajustes", "-e", "rros-settings", "tui", NULL };

		if (run_tool("foot", args, 0) == 127) {
			fputs("rros-settings: no hay navegador ni terminal; usa `rros-settings web --no-open`\n", stderr);
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
	if (!strcmp(cmd, "open"))
		return open_best();
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
	if (!strcmp(cmd, "web")) {
		int port = 0, open_win = 1, idle = 3600;
		const char *token = NULL;

		for (int i = 2; i < argc; i++) {
			if (!strcmp(argv[i], "--port") && i + 1 < argc)
				port = atoi(argv[++i]);
			else if (!strcmp(argv[i], "--no-open"))
				open_win = 0;
			else if (!strcmp(argv[i], "--timeout") && i + 1 < argc)
				idle = atoi(argv[++i]);
			else if (!strcmp(argv[i], "--token") && i + 1 < argc)
				token = argv[++i];
			else {
				usage(stderr);
				return 2;
			}
		}
		if (port < 0 || port > 65535) {
			fputs("rros-settings: puerto no válido\n", stderr);
			return 2;
		}
		return run_web(port, open_win, token, idle);
	}
	usage(stderr);
	return 2;
}
