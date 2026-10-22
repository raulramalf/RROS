/* rros-fetch: ficha del sistema con la identidad de Lily y su presupuesto de recursos. */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/statvfs.h>
#include <sys/utsname.h>
#include <unistd.h>

#include "rros_hw.h"
#include "rros_profile.h"
#include "rros_version.h"
#include "theme.h"

static int use_color;
static int ar = 124, ag = 92, ab = 255;

static void col(int r, int g, int b)
{
	if (use_color)
		printf("\033[38;2;%d;%d;%dm", r, g, b);
}

static void reset(void)
{
	if (use_color)
		fputs("\033[0m", stdout);
}

static void bar(double used, double limit)
{
	int w = 20, fill = limit > 0 ? (int)(used / limit * w + 0.5) : 0;

	if (fill > w)
		fill = w;
	if (used > limit)
		col(255, 107, 129);
	else
		col(ar, ag, ab);
	fputs("[", stdout);
	for (int i = 0; i < w; i++)
		fputs(i < fill ? "█" : "░", stdout);
	fputs("]", stdout);
	reset();
}

static void row(const char *label, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
static void row(const char *label, const char *fmt, ...)
{
	va_list ap;

	col(ar, ag, ab);
	printf("  %-11s", label);
	reset();
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	putchar('\n');
}

static void os_name(char *out, size_t n)
{
	FILE *f = fopen("/etc/os-release", "r");
	char line[256];

	snprintf(out, n, "Lily %s (%s) - desarrollo", RROS_VERSION, RROS_CODENAME);
	if (!f)
		return;
	while (fgets(line, sizeof line, f)) {
		if (strncmp(line, "PRETTY_NAME=", 12) == 0) {
			char *v = line + 12;

			v[strcspn(v, "\r\n")] = '\0';
			if (*v == '"')
				v++;
			if (*v && v[strlen(v) - 1] == '"')
				v[strlen(v) - 1] = '\0';
			if (*v && strstr(v, "Lily"))
				snprintf(out, n, "%s", v);
		}
	}
	fclose(f);
}

static void uptime_str(char *out, size_t n)
{
	FILE *f = fopen("/proc/uptime", "r");
	double up = 0;

	if (f) {
		if (fscanf(f, "%lf", &up) != 1)
			up = 0;
		fclose(f);
	}
	long s = (long)up, d = s / 86400, h = s % 86400 / 3600, m = s % 3600 / 60;

	if (d)
		snprintf(out, n, "%ld d %ld h %ld min", d, h, m);
	else if (h)
		snprintf(out, n, "%ld h %ld min", h, m);
	else
		snprintf(out, n, "%ld min", m);
}

int main(int argc, char **argv)
{
	static const char *logo[] = {
		" ██████╗ ██████╗  ██████╗ ███████╗",
		" ██╔══██╗██╔══██╗██╔═══██╗██╔════╝",
		" ██████╔╝██████╔╝██║   ██║███████╗",
		" ██╔══██╗██╔══██╗██║   ██║╚════██║",
		" ██║  ██║██║  ██║╚██████╔╝███████║",
		" ╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝",
	};
	char profile[64], osn[128], up[64], host[128] = "rros";
	struct rros_conf eff;
	struct rros_hw hw;
	struct utsname u;
	struct statvfs vfs;
	const char *acc, *user = getenv("USER");
	char accbuf[8], themebuf[40];
	long ram_used, ram_limit, disk_limit_gb;
	double disk_used_gb = 0;

	use_color = isatty(STDOUT_FILENO) && !getenv("NO_COLOR");
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--plain"))
			use_color = 0;
		else if (!strcmp(argv[i], "--color"))
			use_color = 1;
		else {
			fputs("uso: rros-fetch [--plain|--color]\n", stderr);
			return 2;
		}
	}
	rros_read_active_profile(profile, sizeof profile);
	rros_conf_init(&eff);
	if (rros_load_effective(&eff, profile) != 0) {
		rros_conf_free(&eff);
		rros_conf_init(&eff);
	}
	{
		struct rros_theme th;
		struct rros_palette pal;
		unsigned v;

		rros_theme_from_conf(&th, &eff);
		rros_theme_palette(&th, &pal);
		snprintf(accbuf, sizeof accbuf, "%s", pal.accent);
		acc = accbuf;
		v = (unsigned)strtoul(acc + 1, NULL, 16);
		ar = (int)((v >> 16) & 255);
		ag = (int)((v >> 8) & 255);
		ab = (int)(v & 255);
		snprintf(themebuf, sizeof themebuf, "%s", th.theme);
	}
	ram_limit = rros_conf_get_long(&eff, "profile.max_idle_ram_mb", 0);
	disk_limit_gb = rros_conf_get_long(&eff, "profile.max_disk_gb", 0);

	gethostname(host, sizeof host);
	os_name(osn, sizeof osn);
	uptime_str(up, sizeof up);
	uname(&u);
	rros_hw_detect(&hw, NULL);
	ram_used = rros_hw_ram_used_mb(NULL);

	putchar('\n');
	for (size_t i = 0; i < sizeof logo / sizeof *logo; i++) {
		col(ar - (int)i * 10 > 0 ? ar - (int)i * 10 : 0, ag, ab);
		puts(logo[i]);
	}
	reset();
	putchar('\n');
	col(ar, ag, ab);
	printf("  %s@%s", user ? user : "usuario", host);
	reset();
	puts("\n  ────────────────────────────");
	row("Sistema", "%s", osn);
	row("Perfil", "%s", profile);
	row("Kernel", "%s %s", u.sysname, u.release);
	row("Uptime", "%s", up);
	row("CPU", "%s (%d)", hw.cpu_model[0] ? hw.cpu_model : "desconocida", hw.cpus);
	row("GPU", "%s", rros_gpu_name(hw.gpu));
	row("Escritorio", "%s", getenv("SWAYSOCK") ? "Lily Shell (Wayland)" : "consola");
	row("Tema", "%s (%s) · acento %s", themebuf, rros_conf_get(&eff, "ui.color_scheme") ?
	    rros_conf_get(&eff, "ui.color_scheme") : "dark", acc);
	if (ram_used >= 0 && ram_limit > 0) {
		col(ar, ag, ab);
		printf("  %-11s", "Memoria");
		reset();
		printf("%ld / %ld MB ", ram_used, ram_limit);
		bar((double)ram_used, (double)ram_limit);
		printf("%s\n", ram_used > ram_limit ? " SUPERADO" : "");
	}
	if (statvfs("/", &vfs) == 0 && disk_limit_gb > 0) {
		disk_used_gb = (double)(vfs.f_blocks - vfs.f_bfree) * (double)vfs.f_frsize / 1073741824.0;
		col(ar, ag, ab);
		printf("  %-11s", "Disco");
		reset();
		printf("%.1f / %ld GB ", disk_used_gb, disk_limit_gb);
		bar(disk_used_gb, (double)disk_limit_gb);
		printf("%s\n", disk_used_gb > (double)disk_limit_gb ? " SUPERADO" : "");
	}
	putchar('\n');
	rros_conf_free(&eff);
	return 0;
}
