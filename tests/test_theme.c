#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "check.h"
#include "theme.h"
#include "rros_schema.h"

static struct rros_conf cur;

/* Renderiza a una cadena (open_memstream). El llamador libera. */
static char *render(rros_render_fn fn, const struct rros_theme *t)
{
	struct rros_palette p;
	char *buf = NULL;
	size_t len = 0;
	FILE *f = open_memstream(&buf, &len);

	rros_theme_palette(t, &p);
	fn(t, &p, &cur, f);
	fclose(f);
	return buf;
}

static void from_text(struct rros_theme *t, const char *text)
{
	rros_conf_free(&cur);
	rros_conf_init(&cur);
	CHECK(rros_conf_parse(&cur, text, NULL) == 0);
	rros_theme_from_conf(t, &cur);
}

int main(void)
{
	struct rros_theme t;
	char *s;

	/* valores por defecto */
	from_text(&t, "");
	CHECK(t.dark == 1);
	CHECK_STR(t.accent, "");
	CHECK_STR(t.theme, "aurora");
	CHECK(t.gap == 8 && t.border == 2);
	CHECK_STR(t.panel_pos, "top");

	/* los ajustes llegan al sway generado */
	from_text(&t, "[ui]\naccent = \"#FF8800\"\ngap = 14\nborder_width = 4\nscale = 150\n"
		      "layout = \"floating\"\nworkspaces = 3\n[input]\nkeymap = \"latam\"\npointer_speed = -50\n");
	s = render(rros_render_sway, &t);
	CHECK(strstr(s, "gaps inner 14") != NULL);
	CHECK(strstr(s, "gaps outer 7") != NULL);
	CHECK(strstr(s, "default_border normal 4") != NULL);   /* ventanas libres: con barra de título */
	CHECK(strstr(s, "#ff8800 #ff8800 #0b1020") != NULL);
	CHECK(strstr(s, "output * scale 1.50") != NULL);
	CHECK(strstr(s, "xkb_layout latam") != NULL);
	CHECK(strstr(s, "pointer_accel -0.50") != NULL);
	CHECK(strstr(s, "floating enable") != NULL);
	CHECK(strstr(s, "workspace number 3") != NULL);
	CHECK(strstr(s, "workspace number 4") == NULL);
	CHECK(strstr(s, RROS_GENERATED) != NULL);
	free(s);

	/* valores peligrosos se descartan: no se puede inyectar comandos en sway */
	from_text(&t, "[ui]\naccent = \"red; exec evil\"\nfont = \"Inter\\nexec evil\"\n[input]\nkeymap = \"es\\nexec x\"\n");
	s = render(rros_render_sway, &t);
	CHECK(strstr(s, "evil") == NULL);
	CHECK(strstr(s, "exec x") == NULL);
	CHECK(strstr(s, "#7c5cff") != NULL); /* acento del tema Aurora */
	CHECK(strstr(s, "font pango:Inter 11") != NULL);
	free(s);

	/* valores fuera de rango se acotan */
	from_text(&t, "[ui]\ngap = 99999\nfont_size = 1\n");
	CHECK(t.gap == 64);
	CHECK(t.font_size == 6);

	/* esquema claro */
	from_text(&t, "[ui]\ncolor_scheme = \"light\"\n");
	s = render(rros_render_foot, &t);
	CHECK(strstr(s, "background=f5f7ff") != NULL);
	free(s);

	/* waybar: posición, reloj y transparencia */
	from_text(&t, "[ui]\npanel_position = \"left\"\npanel_size = 48\nclock_24h = false\ntransparency = 40\n");
	s = render(rros_render_waybar, &t);
	CHECK(strstr(s, "\"position\": \"left\"") != NULL);
	CHECK(strstr(s, "\"width\": 48") != NULL);
	CHECK(strstr(s, "%I:%M %p") != NULL);
	free(s);
	s = render(rros_render_waybar_css, &t);
	CHECK(strstr(s, "rgba(11, 16, 32, 0.60)") != NULL);
	CHECK(strstr(s, "border-right: 2px solid") != NULL);
	free(s);

	/* efectos de SwayFX (sway con efectos): no en off */
	from_text(&t, "[ui]\ncompositor = \"sway\"\nblur = true\nshadows = true\neffects = \"advanced\"\ncorner_radius = 12\n");
	CHECK(t.hyprland == 0);
	s = render(rros_render_effects, &t);
	CHECK(strstr(s, "corner_radius 12") != NULL);
	CHECK(strstr(s, "blur enable") != NULL);
	CHECK(strstr(s, "blur_passes 3") != NULL);
	CHECK(strstr(s, "shadows enable") != NULL);
	free(s);
	from_text(&t, "[ui]\ncompositor = \"sway\"\neffects = \"off\"\nblur = true\n");
	s = render(rros_render_effects, &t);
	CHECK(strstr(s, "blur") == NULL);
	free(s);

	/* ---- temas incluidos ---- */
	from_text(&t, "[ui]\ntheme = \"nord\"\n");
	s = render(rros_render_foot, &t);
	CHECK(strstr(s, "background=2e3440") != NULL);
	CHECK(strstr(s, "selection-background=88c0d0") != NULL); /* acento propio de Nord */
	free(s);
	from_text(&t, "[ui]\ntheme = \"nord\"\naccent = \"#ff0000\"\n");
	s = render(rros_render_foot, &t);
	CHECK(strstr(s, "selection-background=ff0000") != NULL); /* el usuario manda */
	free(s);
	from_text(&t, "[ui]\ntheme = \"gruvbox\"\ncolor_scheme = \"light\"\n");
	s = render(rros_render_foot, &t);
	CHECK(strstr(s, "background=fbf1c7") != NULL);
	free(s);
	{
		struct rros_palette p;

		from_text(&t, "[ui]\ntheme = \"no-existe\"\n");
		CHECK(rros_theme_palette(&t, &p) == 1); /* cae a aurora */
		CHECK_STR(p.bg, "#0b1020");
		from_text(&t, "[ui]\ntheme = \"../../etc/passwd\"\n");
		CHECK_STR(t.theme, "aurora"); /* nombre inválido: nunca se usa como ruta */
	}
	{
		struct rros_theme_info list[64];
		size_t n = rros_theme_list(list, 64);

		CHECK(n >= 7);
		CHECK(list[0].builtin == 1);
	}

	/* ---- panel configurable ---- */
	from_text(&t, "[panel]\nstyle = \"floating\"\nleft = \"workspaces,window\"\nright = \"cpu,hackme,battery\"\n");
	s = render(rros_render_waybar, &t);
	CHECK(strstr(s, "\"modules-left\": [\"hyprland/workspaces\", \"hyprland/window\", \"wlr/taskbar\"]") != NULL);
	CHECK(strstr(s, "\"modules-right\": [\"cpu\", \"battery\"]") != NULL); /* módulo desconocido, descartado */
	CHECK(strstr(s, "hackme") == NULL);
	CHECK(strstr(s, "\"margin\": \"6 8 0 8\"") != NULL);
	free(s);
	s = render(rros_render_waybar_css, &t);
	CHECK(strstr(s, ".modules-left, .modules-center, .modules-right") != NULL);
	free(s);
	from_text(&t, "[panel]\nstyle = \"pill\"\n");
	s = render(rros_render_waybar_css, &t);
	CHECK(strstr(s, "border-radius: 999px") != NULL);
	free(s);

	/* ---- atajos, reglas y programas de inicio del usuario ---- */
	from_text(&t, "[keybind]\nnav = \"Mod+Shift+b exec firefox\"\nmal = \"Mod+x rm -rf\"\n"
		      "[rule]\nff = \"app_id=firefox workspace 2\"\nfloat = \"title=Calculadora floating\"\n"
		      "[autostart]\nsync = \"syncthing -no-browser\"\n");
	s = render(rros_render_sway, &t);
	CHECK(strstr(s, "bindsym $mod+Shift+b exec firefox") != NULL);
	CHECK(strstr(s, "# ignorado (no válido): keybind.mal") != NULL);
	CHECK(strstr(s, "assign [app_id=\"firefox\"] workspace number 2") != NULL);
	CHECK(strstr(s, "for_window [title=\"Calculadora\"] floating enable") != NULL);
	CHECK(strstr(s, "exec syncthing -no-browser") != NULL);
	CHECK(strstr(s, "rm -rf") == NULL);
	free(s);

	/* ---- GTK, mako, fuzzel ---- */
	from_text(&t, "[ui]\ntheme = \"dracula\"\ncolor_scheme = \"dark\"\nfont_size = 13\n");
	s = render(rros_render_gtk_css, &t);
	CHECK(strstr(s, "@define-color window_bg_color #282a36;") != NULL);
	CHECK(strstr(s, "@define-color accent_bg_color #bd93f9;") != NULL);
	CHECK(strstr(s, "@define-color theme_bg_color #282a36;") != NULL);
	free(s);
	s = render(rros_render_gtk_settings, &t);
	CHECK(strstr(s, "gtk-application-prefer-dark-theme=1") != NULL);
	CHECK(strstr(s, "gtk-font-name=Inter 13") != NULL);
	free(s);
	s = render(rros_render_mako, &t);
	CHECK(strstr(s, "background-color=#282a36ff") != NULL);
	CHECK(strstr(s, "border-color=#bd93f9") != NULL);
	free(s);
	s = render(rros_render_fuzzel, &t);
	CHECK(strstr(s, "background=282a36ff") != NULL);
	free(s);

	/* ---- terminal ---- */
	from_text(&t, "[term]\nscrollback = 20000\ncursor = \"beam\"\npadding = 20\n");
	s = render(rros_render_foot, &t);
	CHECK(strstr(s, "lines=20000") != NULL);
	CHECK(strstr(s, "style=beam") != NULL);
	CHECK(strstr(s, "pad=20x16") != NULL);
	free(s);

	/* ---- esenciales de un sistema usable ---- */
	from_text(&t, "");
	s = render(rros_render_sway, &t);
	CHECK(strstr(s, "exec dbus-update-activation-environment --systemd WAYLAND_DISPLAY") != NULL);
	CHECK(strstr(s, "exec rros-polkit-agent") != NULL);
	CHECK(strstr(s, "exec nm-applet --indicator") != NULL);
	CHECK(strstr(s, "exec blueman-applet") != NULL);
	CHECK(strstr(s, "exec udiskie") != NULL);
	CHECK(strstr(s, "exec wl-paste --watch cliphist store") != NULL);
	CHECK(strstr(s, "bindsym $mod+b exec epiphany") != NULL);
	CHECK(strstr(s, "bindsym $mod+e exec thunar") != NULL);
	CHECK(strstr(s, "bindsym $mod+Shift+e exec rros-power") != NULL);
	CHECK(strstr(s, "bindsym $mod+Escape exec swaylock -f") != NULL);
	CHECK(strstr(s, "set $term foot") != NULL);
	/* bloqueo a los 10 min, pantalla a los 15, suspensión a los 30 */
	CHECK(strstr(s, "timeout 600 'swaylock -f'") != NULL);
	CHECK(strstr(s, "timeout 900 'swaymsg \"output * power off\"'") != NULL);
	CHECK(strstr(s, "timeout 1800 'systemctl suspend'") != NULL);
	free(s);

	/* aplicaciones por defecto cambiables, y saneadas */
	from_text(&t, "[apps]\nbrowser = \"firefox\"\nterminal = \"kitty\"\neditor = \"vim; rm -rf /\"\n");
	s = render(rros_render_sway, &t);
	CHECK(strstr(s, "bindsym $mod+b exec firefox") != NULL);
	CHECK(strstr(s, "set $term kitty") != NULL);
	CHECK(strstr(s, "rm -rf") == NULL);                      /* nada de inyección */
	CHECK(strstr(s, "bindsym $mod+Shift+n exec mousepad") != NULL); /* cae al valor por defecto */
	free(s);

	/* bloqueo e inactividad configurables; 0 los desactiva */
	from_text(&t, "[privacy]\nlock_minutes = 0\n[power]\nscreen_off_minutes = 5\nsuspend_minutes = 0\n");
	s = render(rros_render_sway, &t);
	CHECK(strstr(s, "swaylock -f'") != NULL);                /* before-sleep sigue bloqueando */
	CHECK(strstr(s, "timeout 300 'swaymsg") != NULL);
	CHECK(strstr(s, "systemctl suspend") == NULL);
	CHECK(strstr(s, "timeout 600") == NULL);
	free(s);
	from_text(&t, "[privacy]\nlock_minutes = 0\n[power]\nscreen_off_minutes = 0\nsuspend_minutes = 0\n");
	s = render(rros_render_sway, &t);
	CHECK(strstr(s, "exec swayidle") == NULL);
	free(s);

	/* pantalla de bloqueo con la paleta del tema */
	from_text(&t, "[ui]\ntheme = \"nord\"\n");
	s = render(rros_render_swaylock, &t);
	CHECK(strstr(s, "color=2e3440") != NULL);
	CHECK(strstr(s, "key-hl-color=88c0d0") != NULL);
	free(s);

	/* el panel lo lanza rros-panel; no hay bloque bar (sway lo arrancaba antes de tener el bus de sesión) */
	from_text(&t, "");
	s = render(rros_render_sway, &t);
	CHECK(strstr(s, "exec rros-panel") != NULL);
	CHECK(strstr(s, "swaybar_command") == NULL);
	free(s);

	/* panel con botón de aplicaciones y de energía */
	from_text(&t, "");
	s = render(rros_render_waybar, &t);
	CHECK(strstr(s, "\"modules-left\": [\"custom/apps\", \"hyprland/workspaces\", \"hyprland/submap\", \"wlr/taskbar\"]") != NULL);
	CHECK(strstr(s, "\"custom/power\"]") != NULL);
	CHECK(strstr(s, "\"on-click\": \"pavucontrol\"") != NULL);
	free(s);

	/* ---- sway rechaza combinaciones repetidas: nunca debe haber dos iguales ---- */
	{
		static const char *confs[] = {
			"",
			"[keybind]\na = \"Mod+e exec foo\"\nb = \"Mod+B exec bar\"\nc = \"Mod+Shift+e exec baz\"\n"
			"d = \"Mod+r exec nada\"\ne = \"Mod+1 workspace number 7\"\n",
			"[ui]\nworkspaces = 10\n[keybind]\nx = \"Print exec otra\"\n",
		};

		for (size_t k = 0; k < sizeof confs / sizeof *confs; k++) {
			char *line, *save = NULL, *seen[200];
			int n = 0, dup = 0;

			from_text(&t, confs[k]);
			s = render(rros_render_sway, &t);
			for (line = strtok_r(s, "\n", &save); line; line = strtok_r(NULL, "\n", &save)) {
				char *sp;

				if (strncmp(line, "bindsym ", 8) != 0)
					continue;           /* los de dentro de un modo van con sangría */
				sp = strchr(line + 8, ' ');
				if (!sp)
					continue;
				*sp = '\0';
				for (int q = 0; q < n; q++)
					if (strcasecmp(seen[q], line + 8) == 0)
						dup++;
				if (n < 200)
					seen[n++] = line + 8;
			}
			CHECK(dup == 0);
			CHECK(n > 30);
			free(s);
		}
		/* el atajo del usuario sustituye al de RROS, no se suma */
		from_text(&t, "[keybind]\na = \"Mod+e exec foo\"\n");
		s = render(rros_render_sway, &t);
		CHECK(strstr(s, "bindsym $mod+e exec foo") != NULL);
		CHECK(strstr(s, "bindsym $mod+e exec thunar") == NULL);
		CHECK(strstr(s, "bindsym $mod+b exec epiphany") != NULL); /* el resto se mantiene */
		free(s);
	}

	rros_conf_free(&cur);
	/* ---- accesibilidad ---- */
	{
		struct rros_palette pal;
		char *gtk;

		from_text(&t, "[ui]\nhigh_contrast = true\nborder_width = 1\n");
		rros_theme_palette(&t, &pal);
		CHECK(!strcmp(pal.bg, "#000000") && !strcmp(pal.text, "#ffffff"));
		CHECK(t.border >= 3);
		gtk = render(rros_render_gtk_settings, &t);
		CHECK(strstr(gtk, "gtk-theme-name=HighContrast") != NULL);
		free(gtk);
		from_text(&t, "[ui]\nhigh_contrast = true\ncolor_scheme = \"light\"\n");
		rros_theme_palette(&t, &pal);
		CHECK(!strcmp(pal.bg, "#ffffff") && !strcmp(pal.text, "#000000"));

		from_text(&t, "[ui]\nreduce_motion = true\n");
		gtk = render(rros_render_gtk_settings, &t);
		CHECK(strstr(gtk, "gtk-enable-animations=false") != NULL);
		CHECK(strstr(gtk, "HighContrast") == NULL);
		free(gtk);

		from_text(&t, "");
		s = render(rros_render_sway, &t);
		CHECK(strstr(s, "bindsym $mod+Alt+s exec rros-a11y lector") != NULL);
		CHECK(strstr(s, "bindsym $mod+Alt+k exec rros-a11y teclado") != NULL);
		CHECK(strstr(s, "exec rros-a11y lector on") == NULL);
		free(s);
		from_text(&t, "[access]\nscreen_reader = true\nonscreen_keyboard = true\n");
		s = render(rros_render_sway, &t);
		CHECK(strstr(s, "exec rros-a11y lector on") != NULL && strstr(s, "exec rros-a11y teclado on") != NULL);
		free(s);
		/* el usuario puede quitarse el atajo del lector sin que sway se queje de duplicados */
		from_text(&t, "[keybind]\nmio = \"Mod+Alt+s exec foot\"\n");
		s = render(rros_render_sway, &t);
		CHECK(strstr(s, "rros-a11y lector") == NULL);
		free(s);
	}
	/* ---- facilidad: lista de ventanas, barras de título y clic para enfocar ---- */
	from_text(&t, "");
	s = render(rros_render_waybar, &t);
	CHECK(strstr(s, "\"wlr/taskbar\"]") != NULL || strstr(s, "\"wlr/taskbar\",") != NULL);
	CHECK(strstr(s, "\"on-click-middle\": \"close\"") != NULL);
	free(s);
	from_text(&t, "[panel]\ntaskbar = false\n");
	s = render(rros_render_waybar, &t);
	CHECK(strstr(s, "[\"wlr/taskbar\"") == NULL && strstr(s, ", \"wlr/taskbar\"") == NULL);
	free(s);
	from_text(&t, "[ui]\nlayout = \"floating\"\n");
	s = render(rros_render_sway, &t);
	CHECK(strstr(s, "default_floating_border normal") != NULL && strstr(s, "focus_follows_mouse no") != NULL);
	free(s);
	from_text(&t, "[ui]\nlayout = \"tiling\"\n");
	s = render(rros_render_sway, &t);
	CHECK(strstr(s, "default_border pixel") != NULL && strstr(s, "focus_follows_mouse yes") != NULL);
	CHECK(strstr(s, "bindsym $mod+F1 exec rros-settings open bienvenida") != NULL);
	free(s);

	/* ---- Hyprland ---- */
	from_text(&t, "");
	CHECK(t.hyprland == 1);   /* por defecto */
	CHECK_STR(t.wallpaper_style, "aurora");
	setenv("HOME", "/inexistente", 1);
	unsetenv("XDG_DATA_HOME");
	from_text(&t, "[ui]\naccent = \"#FF8800\"\ngap = 14\nborder_width = 3\ncorner_radius = 12\nscale = 125\n"
		      "layout = \"floating\"\nblur = true\nblur_strength = 9\nshadows = true\neffects = \"advanced\"\n"
		      "transparency = 20\nanimation_style = \"rebote\"\nanimation_speed = 200\ntiling_layout = \"master\"\n"
		      "dock = true\nworkspaces = 2\n[input]\nkeymap = \"latam\"\npointer_speed = 50\nnatural_scroll = false\n");
	s = render(rros_render_hyprland, &t);
	CHECK(strstr(s, RROS_GENERATED) != NULL);
	CHECK(strstr(s, "monitor = , preferred, auto, 1.25") != NULL);
	CHECK(strstr(s, "kb_layout = latam") != NULL);
	CHECK(strstr(s, "sensitivity = 0.50") != NULL);
	CHECK(strstr(s, "natural_scroll = false") != NULL);
	CHECK(strstr(s, "follow_mouse = 2") != NULL);                 /* ventanas libres: clic para enfocar */
	CHECK(strstr(s, "gaps_in = 7\n") != NULL && strstr(s, "gaps_out = 14\n") != NULL);
	CHECK(strstr(s, "border_size = 3") != NULL);
	CHECK(strstr(s, "col.active_border = rgba(ff8800ff) rgba(") != NULL);   /* degradado por defecto */
	CHECK(strstr(s, "layout = master") != NULL);
	CHECK(strstr(s, "rounding = 12") != NULL);
	CHECK(strstr(s, "inactive_opacity = 0.90") != NULL);
	CHECK(strstr(s, "dim_inactive = true") != NULL);              /* efectos avanzados */
	CHECK(strstr(s, "    blur {\n        enabled = true\n        size = 9\n        passes = 3") != NULL);
	CHECK(strstr(s, "    shadow {\n        enabled = true") != NULL);
	CHECK(strstr(s, "bezier = rros, 0.68, -0.55, 0.265, 1.55") != NULL);   /* rebote */
	CHECK(strstr(s, "animation = windows, 1, 4, rros, popin 80%") != NULL);  /* 7 al 200 %: 4 décimas */
	CHECK(strstr(s, "windowrule = float on, match:class .*") != NULL);
	CHECK(strstr(s, "windowrulev2") == NULL && strstr(s, "pseudotile") == NULL);
	CHECK(strstr(s, "bind = SUPER, G, layoutmsg, togglesplit") != NULL);
	CHECK(strstr(s, "\ngesture = 3, horizontal, workspace\n") != NULL && strstr(s, "workspace_swipe") == NULL);
	CHECK(strstr(s, "exec-once = rros-dock") != NULL);
	CHECK(strstr(s, "exec-once = swaybg -i /inexistente/.local/share/rros/fondos/aurora-") != NULL);
	CHECK(strstr(s, "bind = SUPER, 2, workspace, 2") != NULL && strstr(s, "workspace, 3") == NULL);
	CHECK(strstr(s, "bind = SUPER, Q, killactive,") != NULL);
	CHECK(strstr(s, "bindm = SUPER, mouse:272, movewindow") != NULL);
	CHECK(strstr(s, "source = ~/.config/hypr/personal.conf") != NULL);
	free(s);
	/* sin efectos ni animaciones; mosaico; fondo liso */
	from_text(&t, "[ui]\neffects = \"off\"\nblur = true\nanimations = false\nlayout = \"tiling\"\nborder_gradient = false\n"
		      "wallpaper_style = \"liso\"\n[input]\ngestures = false\n");
	s = render(rros_render_hyprland, &t);
	CHECK(strstr(s, "    blur {\n        enabled = false") != NULL);
	CHECK(strstr(s, "    shadow {\n        enabled = false") != NULL);
	CHECK(strstr(s, "animations {\n    enabled = false") != NULL);
	CHECK(strstr(s, "follow_mouse = 1") != NULL);
	CHECK(strstr(s, "float on, match:class .*") == NULL);
	CHECK(strstr(s, "gesture") == NULL);
	CHECK(strstr(s, "col.active_border = rgba(7c5cffff)\n") != NULL);
	CHECK(strstr(s, "exec-once = swaybg -c '#0b1020'") != NULL);
	free(s);
	/* pestañas (hybrid): grupos */
	from_text(&t, "[ui]\nlayout = \"hybrid\"\n");
	s = render(rros_render_hyprland, &t);
	CHECK(strstr(s, "windowrule = group set, match:class .*") != NULL && strstr(s, "groupbar {") != NULL);
	free(s);
	/* atajos, reglas e inicio del usuario: traducidos; el atajo propio sustituye al de RROS */
	from_text(&t, "[keybind]\nweb = \"Mod+Shift+b exec firefox #x\"\nmv = \"Mod+F2 move container to workspace number 3\"\n"
		      "q = \"Mod+q exec foot\"\nres = \"Mod+y resize grow width 10px\"\n"
		      "[rule]\ncalc = \"app_id=galculator floating\"\nrol = \"window_role=pop-up floating\"\nop = \"title=Notas opacity 0.8\"\n"
		      "[autostart]\nnube = \"nextcloud --background\"\n");
	s = render(rros_render_hyprland, &t);
	CHECK(strstr(s, "bind = SUPER SHIFT, b, exec, firefox ##x") != NULL);
	CHECK(strstr(s, "bind = SUPER, F2, movetoworkspace, 3") != NULL);
	CHECK(strstr(s, "bind = SUPER, q, exec, foot") != NULL);
	CHECK(strstr(s, "killactive") == NULL);                       /* Super+Q es del usuario */
	CHECK(strstr(s, "# ignorado (no tiene equivalente en Hyprland): keybind.res") != NULL);
	CHECK(strstr(s, "windowrule = float on, match:class ^(galculator)$") != NULL);
	CHECK(strstr(s, "windowrule = opacity 0.8, match:title ^(Notas)$") != NULL);
	CHECK(strstr(s, "# ignorada (no válida en Hyprland): rule.rol") != NULL);
	CHECK(strstr(s, "exec-once = nextcloud --background") != NULL);
	free(s);
	/* waybar con módulos de Hyprland o de sway según el compositor */
	from_text(&t, "[ui]\npanel_position = \"bottom\"\n[panel]\nleft = \"apps,workspaces,window\"\ncenter = \"none\"\n");
	s = render(rros_render_waybar, &t);
	CHECK(strstr(s, "\"hyprland/workspaces\", \"hyprland/window\"") != NULL);
	CHECK(strstr(s, "\"modules-center\": []") != NULL);
	CHECK(strstr(s, "Inicio") != NULL && strstr(s, "{:%d/%m/%Y}") != NULL);   /* barra abajo: botón Inicio y fecha */
	free(s);
	from_text(&t, "[ui]\ncompositor = \"sway\"\n[panel]\nleft = \"workspaces,window\"\n");
	s = render(rros_render_waybar, &t);
	CHECK(strstr(s, "\"sway/workspaces\", \"sway/window\"") != NULL && strstr(s, "hyprland/") == NULL);
	free(s);
	/* hyprlock: colores y fondo del tema */
	from_text(&t, "[ui]\naccent = \"#22d3ee\"\n");
	s = render(rros_render_hyprlock, &t);
	CHECK(strstr(s, "outer_color = rgba(22d3eeff)") != NULL);
	CHECK(strstr(s, "path = /inexistente/.local/share/rros/fondos/aurora-") != NULL);
	CHECK(strstr(s, "placeholder_text = Contraseña") != NULL);
	free(s);

	/* ---- estilos de escritorio ---- */
	{
		const struct rros_desktop_preset *d = rros_desktop_find("clasico");

		CHECK(d != NULL && rros_desktop_find("nada") == NULL && rros_desktop_presets_len == 3);
		/* todas sus claves pasan la validación del esquema */
		for (size_t i = 0; i < rros_desktop_presets_len; i++)
			for (size_t k = 0; k < 12 && rros_desktop_presets[i].keys[k][0]; k++) {
				const struct rros_key *key = rros_schema_find(rros_desktop_presets[i].keys[k][0]);
				char err[200];

				CHECK(key != NULL);
				if (key)
					CHECK(rros_schema_check(key, rros_desktop_presets[i].keys[k][1],
								key->type == RROS_T_STR || key->type == RROS_T_ENUM, err, sizeof err) == 0);
			}
		from_text(&t, "[ui]\npanel_position = \"bottom\"\npanel_size = 40\nlayout = \"floating\"\ndock = false\n"
			      "[panel]\nstyle = \"flat\"\nleft = \"apps,taskbar\"\ncenter = \"none\"\n"
			      "right = \"tray,network,audio,battery,clock,power\"\ntaskbar = true\n");
		CHECK(rros_desktop_current(&cur) == d);
		from_text(&t, "[ui]\npanel_position = \"top\"\n");
		CHECK(rros_desktop_current(&cur) == NULL);
	}
	rros_conf_free(&cur);
	DONE();
}
