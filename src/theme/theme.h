/* Generador del escritorio RROS ("RROS Shell"): sway + waybar + foot desde rros-config. */
#ifndef RROS_THEME_H
#define RROS_THEME_H

#include <stdio.h>
#include "rros_conf.h"

struct rros_palette {
	char bg[8], surface[8], text[8], muted[8], border[8];
	char accent[8], accent_dim[8];
	char red[8], green[8], yellow[8], blue[8], magenta[8], cyan[8];
};

struct rros_theme {
	int dark;
	char accent[8];
	char font[96], font_mono[96];
	int font_size, scale, gap, border, radius, panel_size, workspaces, transparency;
	char panel_pos[8], layout[12], effects[12], keymap[32];
	int animations, blur, shadows, clock_24h, battery_percent;
	int natural_scroll, tap_to_click, dwt, numlock, pointer_speed;
	int repeat_delay, repeat_rate, battery_low;
	int full_compositor;
};

void rros_theme_from_conf(struct rros_theme *t, const struct rros_conf *c);
void rros_theme_palette(const struct rros_theme *t, struct rros_palette *p);

typedef void (*rros_render_fn)(const struct rros_theme *, const struct rros_palette *, FILE *);
void rros_render_sway(const struct rros_theme *, const struct rros_palette *, FILE *);
void rros_render_effects(const struct rros_theme *, const struct rros_palette *, FILE *);
void rros_render_waybar(const struct rros_theme *, const struct rros_palette *, FILE *);
void rros_render_waybar_css(const struct rros_theme *, const struct rros_palette *, FILE *);
void rros_render_foot(const struct rros_theme *, const struct rros_palette *, FILE *);

#define RROS_GENERATED "Generado por rros-theme"

#endif
