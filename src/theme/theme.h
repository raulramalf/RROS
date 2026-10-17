/* Generador del escritorio RROS ("RROS Shell"): Hyprland o sway, waybar, foot, mako, fuzzel, GTK y el fondo. */
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
	int font_size, scale, gap, border, radius, panel_size, workspaces, transparency;
	char panel_pos[8], panel_style[10], layout[12], effects[12], keymap[32];
	char panel_left[128], panel_center[128], panel_right[128];
	char wallpaper[256], wallpaper_mode[8];
	int term_scrollback, term_padding;
	char term_cursor[10];
	int animations, blur, shadows, clock_24h, battery_percent;
	int natural_scroll, tap_to_click, dwt, numlock, pointer_speed;
	int repeat_delay, repeat_rate, battery_low;
	int hyprland;                   /* ui.compositor = hyprland (si no, sway) */
	int animation_speed, blur_strength, dim_inactive, border_gradient, dock, gestures;
	char animation_style[10], tiling[8], wallpaper_style[12], vrr[6];
	char app_term[48], app_browser[48], app_files[48], app_editor[48];
	int lock_min, screen_off_min, suspend_min;
	int high_contrast, reduce_motion, screen_reader, osk;   /* accesibilidad */
	int taskbar;                                            /* lista de ventanas en el panel */
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
void rros_render_sway(const struct rros_theme *, const struct rros_palette *, const struct rros_conf *, FILE *);
void rros_render_effects(const struct rros_theme *, const struct rros_palette *, const struct rros_conf *, FILE *);
void rros_render_waybar(const struct rros_theme *, const struct rros_palette *, const struct rros_conf *, FILE *);
void rros_render_waybar_css(const struct rros_theme *, const struct rros_palette *, const struct rros_conf *, FILE *);
void rros_render_foot(const struct rros_theme *, const struct rros_palette *, const struct rros_conf *, FILE *);
void rros_render_mako(const struct rros_theme *, const struct rros_palette *, const struct rros_conf *, FILE *);
void rros_render_fuzzel(const struct rros_theme *, const struct rros_palette *, const struct rros_conf *, FILE *);
void rros_render_swaylock(const struct rros_theme *, const struct rros_palette *, const struct rros_conf *, FILE *);
void rros_render_gtk_settings(const struct rros_theme *, const struct rros_palette *, const struct rros_conf *, FILE *);
void rros_render_gtk_css(const struct rros_theme *, const struct rros_palette *, const struct rros_conf *, FILE *);
void rros_render_hyprland(const struct rros_theme *, const struct rros_palette *, const struct rros_conf *, FILE *);
void rros_render_hyprlock(const struct rros_theme *, const struct rros_palette *, const struct rros_conf *, FILE *);
/* KDE Plasma: esquema de colores (~/.local/share/color-schemes/RROS-*.colors) */
void rros_render_kde_colors(const struct rros_theme *, const struct rros_palette *, const struct rros_conf *, FILE *);
/* KDE Plasma: ~/.config/kxkbrc con la distribución del teclado */
void rros_render_kxkbrc(const struct rros_theme *, const struct rros_palette *, const struct rros_conf *, FILE *);
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
	const char *keys[12][2];
};
extern const struct rros_desktop_preset rros_desktop_presets[];
extern const size_t rros_desktop_presets_len;
const struct rros_desktop_preset *rros_desktop_find(const char *id);
/* El estilo cuyas claves coinciden todas con la configuración, o NULL (personalizado). */
const struct rros_desktop_preset *rros_desktop_current(const struct rros_conf *c);

#define RROS_GENERATED "Generado por rros-theme"

#endif
