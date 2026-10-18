/* Generador del escritorio RROS sobre KDE Plasma: colores, barras de título, paneles y dock, efectos, entrada,
 * energía, sonido, terminal, atajos, reglas de ventanas y el fondo. */
#ifndef RROS_THEME_H
#define RROS_THEME_H

#include <stdio.h>
#include <stddef.h>
#include "rros_conf.h"

struct rros_palette {
	char bg[8], surface[8], text[8], muted[8], border[8];
	char accent[8], accent_dim[8];
	char red[8], green[8], yellow[8], blue[8], magenta[8], cyan[8];
};

struct rros_theme {
	int dark;
	char theme[40];                 /* nombre del tema (incluido o propio) */
	char accent[8];                 /* acento elegido por el usuario; "" = el del tema */
	char font[96], font_mono[96], icon_theme[48], cursor_theme[48];
	int cursor_size;
	int font_size, scale, radius, panel_size, workspaces, transparency;
	char panel_pos[8], panel_style[10], effects[12], keymap[32];
	char font_hinting[8], font_aa[10];
	char wallpaper[256], wallpaper_mode[8];
	int term_scrollback, term_padding;
	char term_cursor[10];
	int animations, blur, shadows, clock_24h, battery_percent, panel_autohide;
	int natural_scroll, tap_to_click, dwt, numlock, pointer_speed;
	int repeat_delay, repeat_rate, battery_low;
	int animation_speed, blur_strength, dim_inactive, dock, gestures, hot_corners;
	char animation_style[10], wallpaper_style[12], vrr[6];
	int dock_3d, dock_icon, dock_launchpad, buttons_left, global_menu;
	char dock_pos[8], dock_hiding[8];
	char night_light[6], lid_action[10], sound_theme[16];
	int night_temp, event_sounds, max_volume, volume_step, volume_feedback, indexer;
	char app_term[48], app_browser[48], app_files[48], app_editor[48];
	int lock_min, screen_off_min, suspend_min;
	int high_contrast, reduce_motion, screen_reader, osk;   /* accesibilidad */
	int taskbar;                                            /* ventanas abiertas en la barra (estilo clásico) */
	int soft_render;   /* sin aceleración gráfica (máquina virtual sin 3D): efectos apagados */
};

struct rros_theme_info {
	char name[40];
	int builtin;
};

void rros_theme_from_conf(struct rros_theme *t, const struct rros_conf *c);
/* 0 si se usó el tema pedido; 1 si no existe y se cayó a "aurora". */
int rros_theme_palette(const struct rros_theme *t, struct rros_palette *p);
/* Lista los temas incluidos y los propios. Devuelve cuántos escribió. */
size_t rros_theme_list(struct rros_theme_info *out, size_t max);
/* Carpeta de temas propios del usuario. */
int rros_themes_dir(char *buf, size_t n);
/* Crea un tema propio a partir de otro (incluido o propio). 0 ok, <0 con mensaje. */
int rros_theme_new(const char *name, const char *base, char *err, size_t en);

/* Cambia un color de un tema propio (variante "dark" o "light"). 0 ok, <0 con mensaje. */
int rros_theme_set_color(const char *name, const char *variant, const char *key, const char *hex,
			 char *err, size_t en);

typedef void (*rros_render_fn)(const struct rros_theme *, const struct rros_palette *,
			       const struct rros_conf *, FILE *);
/* KDE Plasma: esquema de colores (~/.local/share/color-schemes/RROS-*.colors) */
void rros_render_kde_colors(const struct rros_theme *, const struct rros_palette *, const struct rros_conf *, FILE *);
/* KDE Plasma: guion de plasmashell que crea la barra superior y el dock según la configuración */
void rros_render_plasma_layout(const struct rros_theme *, const struct rros_palette *, const struct rros_conf *, FILE *);
/* KDE Plasma: perfil y colores de Konsole */
void rros_render_konsole_profile(const struct rros_theme *, const struct rros_palette *, const struct rros_conf *, FILE *);
void rros_render_konsole_colors(const struct rros_theme *, const struct rros_palette *, const struct rros_conf *, FILE *);

/* Ajustes de Plasma que van a archivos de KDE (~/.config/kwinrc, kdeglobals...): se mezclan con lo que ya hay,
 * así no se pierde lo que el usuario cambie en las Preferencias del sistema. */
struct rros_ini_edit {
	const char *file;    /* relativo a la carpeta de configuración: "kwinrc" */
	char group[64];      /* "Plugins" o "AC][Display" (grupos anidados de KDE) */
	const char *key;
	char value[160];
	int del_group;       /* 1: borra los grupos (group puede acabar en '*') */
	int merge_list;      /* 1: lista con comas; se conservan las entradas que no son rros-* */
};
#define RROS_MAX_EDITS 400
/* Rellena las ediciones. Devuelve cuántas. */
size_t rros_plasma_edits(const struct rros_theme *t, const struct rros_palette *p, const struct rros_conf *c,
			 struct rros_ini_edit *out, size_t max);
/* Aplica las ediciones de un archivo (las demás se saltan) en path. 0 ok. */
int rros_ini_apply(const char *path, const char *file, const struct rros_ini_edit *e, size_t n);
/* Lee un valor de un archivo INI de KDE ("" si no está). */
int rros_ini_get(const char *path, const char *group, const char *key, char *out, size_t n);
/* Archivos de KDE que tocan las ediciones (lista terminada en NULL). */
extern const char *const rros_kde_files[];
/* Lanzadores de atajos y de programas de inicio (~/.local/share/applications, ~/.config/autostart). 0 ok. */
int rros_write_launchers(const struct rros_conf *c, const char *apps_dir, const char *autostart_dir);
/* Nombre del tema de escritorio de Plasma según el dock: "rros" (2D) o "rros-3d". */
const char *rros_desktoptheme(const struct rros_theme *t);
/* KDE Plasma: tema de barras de título (Aurorae) «RROS» en dir, con los colores del tema. 0 ok. */
int rros_write_aurorae(const char *dir, const struct rros_theme *t, const struct rros_palette *p);

/* Fondo de pantalla: el elegido (ui.wallpaper) o uno generado con la paleta (ui.wallpaper_style).
 * Escribe en out la ruta de la imagen, o "" si el fondo es un color liso. */
void rros_wallpaper_path(const struct rros_theme *t, const struct rros_palette *p, char *out, size_t n);
/* Genera el PNG del estilo con la paleta. 0 ok. */
int rros_wallpaper_render(const char *style, const struct rros_palette *p, int w, int h, const char *path);
/* "#rrggbb" -> componentes */
void rros_hex_rgb(const char *hex, int *r, int *g, int *b);

/* Estilos de escritorio: un conjunto de claves (panel, ventanas, dock) que se escriben de una vez. */
struct rros_desktop_preset {
	const char *id, *label, *desc;
	const char *keys[14][2];
};
extern const struct rros_desktop_preset rros_desktop_presets[];
extern const size_t rros_desktop_presets_len;
const struct rros_desktop_preset *rros_desktop_find(const char *id);
/* El estilo cuyas claves coinciden todas con la configuración, o NULL (personalizado). */
const struct rros_desktop_preset *rros_desktop_current(const struct rros_conf *c);

#define RROS_GENERATED "Generado por rros-theme"

#endif
