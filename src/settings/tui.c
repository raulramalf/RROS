/* rros-settings tui: ajustes en la terminal (Lite, SSH). */
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "rros_profile.h"
#include "rros_userconf.h"
#include "settings.h"
#include "rros_schema.h"
#include "theme.h"

enum { K_NONE = 256, K_UP, K_DOWN, K_LEFT, K_RIGHT, K_PGUP, K_PGDN, K_ENTER, K_ESC, K_TAB, K_STAB, K_BS };
enum { R_STATIC, R_DYN, R_ADD };

struct row {
	int kind;
	const struct rros_key *k;
	char key[96];
	char id[40];
	enum rros_family fam;
};

#define MAXROWS 160
#define MAXCATS 24

static struct termios orig;
static int raw_on;
static volatile sig_atomic_t resized;

static char cats[MAXCATS][16];
static const char *catlabels[MAXCATS];
static size_t ncats;
static int ar = 124, ag = 92, ab = 255;

static void term_restore(void)
{
	if (raw_on) {
		fputs("\033[?25h\033[?1049l", stdout);
		fflush(stdout);
		tcsetattr(STDIN_FILENO, TCSANOW, &orig);
		raw_on = 0;
	}
}

static void on_term(int sig)
{
	(void)sig;
	term_restore();
	_exit(130);
}

static void on_winch(int sig)
{
	(void)sig;
	resized = 1;
}

static int term_raw(void)
{
	struct termios t;
	struct sigaction sa;

	if (tcgetattr(STDIN_FILENO, &orig) != 0)
		return -1;
	t = orig;
	t.c_lflag &= ~(tcflag_t)(ICANON | ECHO | ISIG | IEXTEN);
	t.c_iflag &= ~(tcflag_t)(IXON | ICRNL);
	t.c_cc[VMIN] = 1;
	t.c_cc[VTIME] = 0;
	if (tcsetattr(STDIN_FILENO, TCSANOW, &t) != 0)
		return -1;
	raw_on = 1;
	atexit(term_restore);
	memset(&sa, 0, sizeof sa);
	sa.sa_handler = on_term;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGHUP, &sa, NULL);
	sa.sa_handler = on_winch; /* sin SA_RESTART: read() se interrumpe y se redibuja */
	sigaction(SIGWINCH, &sa, NULL);
	fputs("\033[?1049h\033[?25l", stdout);
	return 0;
}

static int read_byte(int timeout_ds)
{
	struct termios t;
	unsigned char c;
	ssize_t n;

	if (timeout_ds >= 0) {
		tcgetattr(STDIN_FILENO, &t);
		t.c_cc[VMIN] = 0;
		t.c_cc[VTIME] = (cc_t)timeout_ds;
		tcsetattr(STDIN_FILENO, TCSANOW, &t);
	}
	n = read(STDIN_FILENO, &c, 1);
	if (timeout_ds >= 0) {
		t.c_cc[VMIN] = 1;
		t.c_cc[VTIME] = 0;
		tcsetattr(STDIN_FILENO, TCSANOW, &t);
	}
	if (n == 1)
		return c;
	if (n < 0 && errno == EINTR)
		return -2;
	return -1;
}

static int read_key(void)
{
	int c = read_byte(-1);

	if (c == -2)
		return K_NONE;
	if (c < 0)
		return K_ESC; /* EOF: salir */
	if (c == '\r' || c == '\n')
		return K_ENTER;
	if (c == '\t')
		return K_TAB;
	if (c == 127 || c == 8)
		return K_BS;
	if (c == 3)
		return K_ESC;
	if (c != 27)
		return c;
	c = read_byte(1);
	if (c != '[' && c != 'O')
		return K_ESC;
	c = read_byte(1);
	switch (c) {
	case 'A': return K_UP;
	case 'B': return K_DOWN;
	case 'C': return K_RIGHT;
	case 'D': return K_LEFT;
	case 'Z': return K_STAB;
	case '5': read_byte(1); return K_PGUP;
	case '6': read_byte(1); return K_PGDN;
	default:  return K_NONE;
	}
}

/* ---- modelo de filas ---- */

static void setup_cats(void)
{
	ncats = model_categories(cats, MAXCATS - 3);   /* deja sitio a atajos, reglas e inicio */
	for (size_t i = 0; i < ncats; i++)
		catlabels[i] = model_category_label(cats[i]);
	snprintf(cats[ncats], 16, "keybind"); catlabels[ncats++] = "Atajos";
	snprintf(cats[ncats], 16, "rule"); catlabels[ncats++] = "Reglas de ventana";
	snprintf(cats[ncats], 16, "autostart"); catlabels[ncats++] = "Inicio automático";
}

static enum rros_family fam_of_cat(const char *cat)
{
	return !strcmp(cat, "keybind") ? RROS_FAM_KEYBIND : !strcmp(cat, "rule") ? RROS_FAM_RULE :
	       !strcmp(cat, "autostart") ? RROS_FAM_AUTOSTART : RROS_FAM_NONE;
}

static size_t build_rows(const struct model *m, size_t cat, struct row *rows)
{
	size_t n = 0;
	enum rros_family fam = fam_of_cat(cats[cat]);
	char prefix[24];

	snprintf(prefix, sizeof prefix, "%s.", cats[cat]);
	if (fam == RROS_FAM_NONE) {
		for (size_t i = 0; i < rros_schema_len && n < MAXROWS; i++) {
			const struct rros_key *k = &rros_schema[i];

			if (k->profile_only || strncmp(k->key, prefix, strlen(prefix)))
				continue;
			rows[n].kind = R_STATIC;
			rows[n].k = k;
			snprintf(rows[n].key, sizeof rows[n].key, "%s", k->key);
			rows[n].fam = RROS_FAM_NONE;
			n++;
		}
		return n;
	}
	for (size_t i = 0; i < m->eff.n && n < MAXROWS - 1; i++) {
		const struct rros_kv *kv = &m->eff.items[i];

		if (strncmp(kv->key, prefix, strlen(prefix)))
			continue;
		rows[n].kind = R_DYN;
		rows[n].k = NULL;
		snprintf(rows[n].key, sizeof rows[n].key, "%s", kv->key);
		snprintf(rows[n].id, sizeof rows[n].id, "%.39s", kv->key + strlen(prefix));
		rows[n].fam = fam;
		n++;
	}
	rows[n].kind = R_ADD;
	rows[n].k = NULL;
	rows[n].key[0] = '\0';
	rows[n].fam = fam;
	return n + 1;
}

/* ---- dibujo ---- */

struct ui {
	struct model m;
	size_t cat, sel, scroll;
	char status[300];
	int status_err;
	int auto_apply;
	int rows_n, cols_n;
};

static void size_term(struct ui *u)
{
	struct winsize w;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 20 && w.ws_row > 8) {
		u->cols_n = w.ws_col;
		u->rows_n = w.ws_row;
	} else {
		u->cols_n = 80;
		u->rows_n = 24;
	}
}

static void accent(void)
{
	printf("\033[38;2;%d;%d;%dm", ar, ag, ab);
}

static void value_text(const struct ui *u, const struct row *r, char *out, size_t n)
{
	const char *v = rros_conf_get(&u->m.eff, r->key);

	if (r->kind == R_ADD) {
		snprintf(out, n, "+ añadir");
		return;
	}
	if (!v)
		v = "";
	if (r->kind == R_STATIC && r->k->type == RROS_T_BOOL)
		snprintf(out, n, "%s", !strcmp(v, "true") ? "[x] activado" : "[ ] desactivado");
	else if (r->kind == R_STATIC && r->k->type == RROS_T_INT)
		snprintf(out, n, "%s  (%ld–%ld)", v, r->k->min, r->k->max);
	else
		snprintf(out, n, "%s", *v ? v : "(vacío)");
}

static void draw(struct ui *u)
{
	struct row rows[MAXROWS];
	size_t nrows = build_rows(&u->m, u->cat, rows);
	int list_h, left_w = 20, i;

	size_term(u);
	list_h = u->rows_n - 6;
	if (u->sel >= nrows)
		u->sel = nrows ? nrows - 1 : 0;
	if (u->sel < u->scroll)
		u->scroll = u->sel;
	if (u->sel >= u->scroll + (size_t)list_h)
		u->scroll = u->sel - (size_t)list_h + 1;

	fputs("\033[H\033[2J", stdout);
	accent();
	printf("\033[1m RROS Ajustes\033[0m  perfil %s  ·  %s\n", u->m.profile, "los cambios se guardan al instante");
	printf("\033[90m%.*s\033[0m\n", u->cols_n, "────────────────────────────────────────────────────────────────────────────────────────────────────────────────────");
	for (i = 0; i < list_h; i++) {
		size_t ci = (size_t)i, ri = u->scroll + (size_t)i;

		printf("\033[%d;1H", 3 + i);
		if (ci < ncats) {
			if (ci == u->cat) {
				accent();
				printf("\033[1m> %-*.*s\033[0m", left_w - 2, left_w - 2, catlabels[ci]);
			} else {
				printf("  %-*.*s", left_w - 2, left_w - 2, catlabels[ci]);
			}
		} else {
			printf("%*s", left_w, "");
		}
		if (ri < nrows) {
			char val[200];
			const char *src = rows[ri].kind == R_STATIC ? model_source(&u->m, rows[ri].key) : "profile";
			int width = u->cols_n - left_w - 3;
			int kw = rows[ri].kind == R_DYN ? 22 : 22;

			value_text(u, &rows[ri], val, sizeof val);
			printf("\033[%d;%dH", 3 + i, left_w + 2);
			if (ri == u->sel) {
				accent();
				fputs("\033[1m\033[7m", stdout);
			}
			printf("%-*.*s %s%-.*s", kw, kw, rows[ri].kind == R_ADD ? "" : rows[ri].key + (rows[ri].kind == R_DYN ? strlen(cats[u->cat]) + 1 : 0),
			       !strcmp(src, "user") ? "*" : " ", width - kw - 2 > 0 ? width - kw - 2 : 0, val);
			fputs("\033[0m", stdout);
		}
	}
	/* descripción de la fila */
	printf("\033[%d;1H\033[90m%.*s\033[0m", u->rows_n - 3, u->cols_n, "────────────────────────────────────────────────────────────────────────────────────────────────────────────────────");
	if (nrows) {
		const struct row *r = &rows[u->sel];

		printf("\033[%d;1H %.*s", u->rows_n - 2, u->cols_n - 2,
		       r->kind == R_STATIC ? r->k->desc : rros_family_desc(r->fam));
	}
	printf("\033[%d;1H", u->rows_n - 1);
	if (u->status[0]) {
		if (u->status_err)
			printf("\033[31m %.*s\033[0m", u->cols_n - 2, u->status);
		else
			printf("\033[32m %.*s\033[0m", u->cols_n - 2, u->status);
	}
	printf("\033[%d;1H\033[90m Tab categoría · ↑↓ elegir · ←→ cambiar · Enter editar · d restablecer · / buscar · a aplicar · q salir\033[0m", u->rows_n);
	fflush(stdout);
}

/* ---- edición ---- */

static int edit_line(struct ui *u, const char *prompt, const char *initial, char *out, size_t n)
{
	size_t len = strlen(initial);

	snprintf(out, n, "%s", initial);
	len = strlen(out);
	for (;;) {
		int k;

		printf("\033[%d;1H\033[2K\033[1m %s\033[0m %s\033[7m \033[0m", u->rows_n - 1, prompt, out);
		fflush(stdout);
		k = read_key();
		if (k == K_ENTER)
			return 0;
		if (k == K_ESC)
			return -1;
		if (k == K_BS) {
			if (len)
				out[--len] = '\0';
		} else if (k >= 32 && k < 127 && len + 1 < n) {
			out[len++] = (char)k;
			out[len] = '\0';
		}
	}
}

static void reload(struct ui *u)
{
	model_free(&u->m);
	model_load(&u->m);
}

static void set_status(struct ui *u, int err, const char *fmt, const char *a, const char *b)
{
	snprintf(u->status, sizeof u->status, fmt, a, b);
	u->status_err = err;
}

static void apply_now(struct ui *u)
{
	char *args[] = { "rros-theme", "apply", "--quiet", NULL };

	if (run_tool("rros-theme", args, 1) == 0)
		set_status(u, 0, "escritorio actualizado%s%s", "", "");
	else
		set_status(u, 1, "no se pudo aplicar el tema%s%s", "", "");
}

static int affects_desktop(const char *key)
{
	return !strncmp(key, "ui.", 3) || !strncmp(key, "panel.", 6) || !strncmp(key, "term.", 5) ||
	       !strncmp(key, "input.", 6) || !strncmp(key, "keybind.", 8) || !strncmp(key, "rule.", 5) ||
	       !strncmp(key, "autostart.", 10) || !strncmp(key, "access.", 7);
}

/* Los ajustes del sistema los aplica rros-apply como root: sin root solo se explica cómo hacerlo. */
static int system_key_blocked(struct ui *u, const char *key)
{
	if (!rros_key_system_scope(key) || geteuid() == 0)
		return 0;
	set_status(u, 1, "ajuste del sistema: sudo rros-config set --system %s VALOR && sudo rros-apply --services%s", key, "");
	return 1;
}

static void apply_system(void)
{
	char *args[] = { "rros-apply", "--services", NULL };

	run_tool("rros-apply", args, 1);
}

static void do_set(struct ui *u, const char *key, const char *val)
{
	char err[300];
	int sys = rros_key_system_scope(key);

	if (system_key_blocked(u, key))
		return;
	if (rros_user_set(key, val, sys, err, sizeof err)) {
		set_status(u, 1, "%s%s", err, "");
		return;
	}
	if (sys)
		apply_system();
	reload(u);
	set_status(u, 0, "guardado: %s = %s", key, val);
	if (u->auto_apply && affects_desktop(key))
		apply_now(u);
}

static void change(struct ui *u, const struct row *r, int dir, int big)
{
	const char *cur = rros_conf_get(&u->m.eff, r->key);
	char buf[64];

	if (r->kind != R_STATIC)
		return;
	if (r->k->type == RROS_T_BOOL) {
		do_set(u, r->key, cur && !strcmp(cur, "true") ? "false" : "true");
	} else if (r->k->type == RROS_T_INT) {
		long v = cur ? strtol(cur, NULL, 10) : r->k->min;

		v += (long)dir * (big ? 10 : 1);
		if (v < r->k->min)
			v = r->k->min;
		if (v > r->k->max)
			v = r->k->max;
		snprintf(buf, sizeof buf, "%ld", v);
		do_set(u, r->key, buf);
	} else if (r->k->type == RROS_T_ENUM) {
		char list[256], *tok, *save = NULL, *items[32];
		int n = 0, idx = 0;

		snprintf(list, sizeof list, "%s", r->k->choices);
		for (tok = strtok_r(list, "|", &save); tok && n < 32; tok = strtok_r(NULL, "|", &save)) {
			if (cur && !strcmp(tok, cur))
				idx = n;
			items[n++] = tok;
		}
		if (n)
			do_set(u, r->key, items[(idx + dir + n) % n]);
	}
}

static void enter(struct ui *u, const struct row *r)
{
	char val[256], id[64], key[160];

	if (r->kind == R_STATIC && r->k->type != RROS_T_STR && r->k->type != RROS_T_INT) {
		change(u, r, 1, 0);
		return;
	}
	if (r->kind == R_ADD) {
		if (edit_line(u, "identificador (a-z, 0-9, _ -):", "", id, sizeof id))
			return;
		if (edit_line(u, rros_family_desc(r->fam), "", val, sizeof val))
			return;
		snprintf(key, sizeof key, "%s.%s", cats[u->cat], id);
		do_set(u, key, val);
		return;
	}
	{
		const char *cur = rros_conf_get(&u->m.eff, r->key);

		if (edit_line(u, "nuevo valor:", cur ? cur : "", val, sizeof val))
			return;
		do_set(u, r->key, val);
	}
}

static void search(struct ui *u)
{
	char q[64];
	size_t total = ncats;

	if (edit_line(u, "buscar:", "", q, sizeof q) || !q[0])
		return;
	/* empieza tras la fila actual, recorre el resto de categorías y vuelve al principio de la actual */
	for (size_t off = 0; off <= total; off++) {
		size_t c = (u->cat + off) % total;
		struct row rows[MAXROWS];
		size_t n = build_rows(&u->m, c, rows);
		size_t start = (off == 0) ? u->sel + 1 : 0;
		size_t stop = (off == total) ? u->sel + 1 : n;

		for (size_t i = start; i < stop && i < n; i++) {
			const char *text = rows[i].kind == R_STATIC ? rows[i].k->desc : "";

			if (rows[i].kind != R_ADD && (strstr(rows[i].key, q) || strstr(text, q))) {
				u->cat = c;
				u->sel = i;
				u->status[0] = '\0';
				return;
			}
		}
	}
	set_status(u, 1, "no se encontró '%s'", q, "");
}

int run_tui(int auto_apply)
{
	struct ui u;
	struct rros_theme th;
	struct rros_palette pal;
	int quit = 0;

	if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
		fputs("rros-settings: la interfaz de terminal necesita un terminal interactivo\n", stderr);
		return 1;
	}
	memset(&u, 0, sizeof u);
	if (model_load(&u.m) != 0) {
		fputs("rros-settings: no se pudo cargar el perfil\n", stderr);
		return 1;
	}
	{
		unsigned v;

		rros_theme_from_conf(&th, &u.m.eff);
		rros_theme_palette(&th, &pal);
		v = (unsigned)strtoul(pal.accent + 1, NULL, 16);
		ar = (int)((v >> 16) & 255);
		ag = (int)((v >> 8) & 255);
		ab = (int)(v & 255);
	}
	u.auto_apply = auto_apply;
	setup_cats();
	if (term_raw() != 0) {
		fputs("rros-settings: no se pudo configurar el terminal\n", stderr);
		model_free(&u.m);
		return 1;
	}
	while (!quit) {
		struct row rows[MAXROWS];
		size_t nrows = build_rows(&u.m, u.cat, rows);
		int k;

		draw(&u);
		k = read_key();
		if (resized) {
			resized = 0;
			continue;
		}
		switch (k) {
		case K_ESC: case 'q': quit = 1; break;
		case K_TAB: u.cat = (u.cat + 1) % ncats; u.sel = u.scroll = 0; u.status[0] = '\0'; break;
		case K_STAB: u.cat = (u.cat + ncats - 1) % ncats; u.sel = u.scroll = 0; u.status[0] = '\0'; break;
		case K_UP: case 'k': if (u.sel) u.sel--; break;
		case K_DOWN: case 'j': if (u.sel + 1 < nrows) u.sel++; break;
		case K_PGUP: u.sel = u.sel > 10 ? u.sel - 10 : 0; break;
		case K_PGDN: u.sel = u.sel + 10 < nrows ? u.sel + 10 : nrows - 1; break;
		case K_RIGHT: case 'l': if (nrows) change(&u, &rows[u.sel], 1, 0); break;
		case K_LEFT: case 'h': if (nrows) change(&u, &rows[u.sel], -1, 0); break;
		case '+': if (nrows) change(&u, &rows[u.sel], 1, 1); break;
		case '-': if (nrows) change(&u, &rows[u.sel], -1, 1); break;
		case K_ENTER: case ' ': if (nrows) enter(&u, &rows[u.sel]); break;
		case 'd': {
			char err[300];

			if (nrows && rows[u.sel].kind != R_ADD && !system_key_blocked(&u, rows[u.sel].key)) {
				if (rros_user_unset(rows[u.sel].key, rros_key_system_scope(rows[u.sel].key), err, sizeof err)) {
					set_status(&u, 1, "%s%s", err, "");
				} else {
					reload(&u);
					set_status(&u, 0, "restablecido: %s%s", rows[u.sel].key, "");
					if (u.auto_apply && affects_desktop(rows[u.sel].key))
						apply_now(&u);
				}
			}
			break;
		}
		case '/': search(&u); break;
		case 'a': apply_now(&u); break;
		default: break;
		}
	}
	term_restore();
	model_free(&u.m);
	return 0;
}
