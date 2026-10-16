/* rros: comando unificado. Despacha a rros-<subcomando> y ofrece `doctor`. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "rros_conf.h"
#include "rros_profile.h"
#include "rros_version.h"

static char self_dir[512] = "";

static void exec_tool(const char *tool, char *const argv[])
{
	char path[640];

	if (self_dir[0]) {
		snprintf(path, sizeof path, "%s/%s", self_dir, tool);
		if (access(path, X_OK) == 0)
			execv(path, argv);
	}
	execvp(tool, argv);
}

/* Ejecuta una herramienta y devuelve su código de salida. */
static int run_quiet(const char *tool, char *const args[])
{
	pid_t pid;
	int st;

	fflush(stdout);
	pid = fork();
	if (pid < 0)
		return -1;
	if (pid == 0) {
		if (!freopen("/dev/null", "w", stdout))
			_exit(127);
		exec_tool(tool, args);
		_exit(127);
	}
	if (waitpid(pid, &st, 0) < 0)
		return -1;
	return WIFEXITED(st) ? WEXITSTATUS(st) : -1;
}

static int bad;
static int color;

static void report(int ok, const char *msg)
{
	const char *plain = ok == 1 ? "[ok]" : ok == 0 ? "[x] " : "[!] ";
	const char *sym = ok == 1 ? "✓" : ok == 0 ? "✗" : "!";
	const char *c = ok == 1 ? "\033[32m" : ok == 0 ? "\033[31m" : "\033[33m";

	if (color)
		printf("  %s%s\033[0m %s\n", c, sym, msg);
	else
		printf("  %s %s\n", plain, msg);
	if (ok == 0)
		bad = 1;
}

static int doctor(void)
{
	char profile[64], msg[256];
	struct rros_conf eff;
	char *v_args[] = { "rros-config", "validate", NULL };
	char *b_args[] = { "rros-budget", "ram", NULL };
	int rc;

	color = isatty(STDOUT_FILENO) && !getenv("NO_COLOR");
	rros_read_active_profile(profile, sizeof profile);
	printf("RROS %s · diagnóstico\n", RROS_VERSION);

	rros_conf_init(&eff);
	rc = rros_load_effective(&eff, profile);
	snprintf(msg, sizeof msg, rc == 0 ? "perfil '%s' cargado" : "perfil '%s' no se pudo cargar", profile);
	report(rc == 0, msg);

	rc = run_quiet("rros-config", v_args);
	report(rc == 0, rc == 0 ? "configuración válida" : "configuración con errores (rros-config validate)");

	rc = run_quiet("rros-budget", b_args);
	report(rc == 0 ? 1 : rc == 1 ? 0 : 2,
	       rc == 0 ? "RAM dentro del presupuesto" : rc == 1 ? "RAM por encima del presupuesto" :
	       "no se pudo medir la RAM");

	report(access("/usr/bin/snap", F_OK) != 0 ? 1 : 2,
	       access("/usr/bin/snap", F_OK) != 0 ? "sin snap" : "snap presente (RROS no lo usa)");
	report(!rros_conf_get_bool(&eff, "privacy.telemetry", 0) ? 1 : 2,
	       !rros_conf_get_bool(&eff, "privacy.telemetry", 0) ? "telemetría desactivada" :
	       "telemetría activada");
	rros_conf_free(&eff);
	return bad ? 1 : 0;
}

static void usage(void)
{
	fputs("RROS " RROS_VERSION " (" RROS_CODENAME ")\n"
	      "uso: rros COMANDO [argumentos]\n"
	      "  info      ficha del sistema          (rros-fetch)\n"
	      "  config    personalización            (rros-config)\n"
	      "  theme     tema del escritorio        (rros-theme)\n"
	      "  budget    presupuesto de RAM/disco   (rros-budget)\n"
	      "  hw        hardware y perfil          (rros-hwdetect)\n"
	      "  install   plan de instalación        (rros-installer)\n"
	      "  doctor    diagnóstico del sistema\n"
	      "  version\n", stderr);
}

int main(int argc, char **argv)
{
	static const struct { const char *name, *tool; } map[] = {
		{ "info", "rros-fetch" }, { "config", "rros-config" }, { "theme", "rros-theme" },
		{ "budget", "rros-budget" }, { "hw", "rros-hwdetect" }, { "install", "rros-installer" },
	};
	char *slash = strrchr(argv[0], '/');

	if (slash) {
		size_t n = (size_t)(slash - argv[0]);

		if (n < sizeof self_dir) {
			memcpy(self_dir, argv[0], n);
			self_dir[n] = '\0';
		}
	}
	if (argc < 2) {
		usage();
		return 2;
	}
	if (!strcmp(argv[1], "version")) {
		puts("RROS " RROS_VERSION " (" RROS_CODENAME ")");
		return 0;
	}
	if (!strcmp(argv[1], "doctor"))
		return doctor();
	for (size_t i = 0; i < sizeof map / sizeof *map; i++) {
		if (!strcmp(argv[1], map[i].name)) {
			argv[1] = (char *)map[i].tool;
			exec_tool(map[i].tool, argv + 1);
			perror(map[i].tool);
			return 127;
		}
	}
	usage();
	return 2;
}
