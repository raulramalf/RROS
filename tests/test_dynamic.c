#include <string.h>

#include "check.h"
#include "rros_schema.h"

static int kb(const char *v, char *out)
{
	return rros_keybind_to_sway(v, out, 512);
}

int main(void)
{
	char out[512], err[256];

	/* atajos */
	CHECK(kb("Mod+Shift+b exec firefox", out) == 0);
	CHECK_STR(out, "bindsym $mod+Shift+b exec firefox");
	CHECK(kb("Ctrl+Alt+Delete exec systemctl reboot", out) == 0);
	CHECK_STR(out, "bindsym Ctrl+Alt+Delete exec systemctl reboot");
	CHECK(kb("Mod+1 workspace number 1", out) == 0);
	CHECK(kb("Print exec rros-shot", out) == 0);
	CHECK(kb("Mod+x", out) == -1);                    /* sin comando */
	CHECK(kb("exec firefox", out) == -1);             /* "exec" sería la tecla y "firefox" no es comando */
	CHECK(kb("Mod+x rm -rf /", out) == -1);           /* comando no permitido */
	CHECK(kb("Mod++b exec x", out) == -1);
	CHECK(kb("Mod+ exec x", out) == -1);
	CHECK(kb("Mod+b; exec x", out) == -1);
	CHECK(kb("Mod+b exec {evil}", out) == -1);
	CHECK(kb("Mod+b exec x\nbindsym y exec z", out) == -1); /* sin inyección de líneas */
	CHECK(kb("", out) == -1);
	CHECK(rros_keybind_to_sway("Mod+b exec " "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", out, sizeof out) == -1);
	CHECK(rros_keybind_to_sway("Mod+b exec firefox", out, 8) == -1); /* búfer pequeño */

	/* reglas */
	CHECK(rros_rule_to_sway("app_id=firefox workspace 2", out, sizeof out) == 0);
	CHECK_STR(out, "assign [app_id=\"firefox\"] workspace number 2");
	CHECK(rros_rule_to_sway("class=Steam floating", out, sizeof out) == 0);
	CHECK_STR(out, "for_window [class=\"Steam\"] floating enable");
	CHECK(rros_rule_to_sway("title=Video opacity 0.85", out, sizeof out) == 0);
	CHECK_STR(out, "for_window [title=\"Video\"] opacity 0.85");
	CHECK(rros_rule_to_sway("app_id=mpv border none", out, sizeof out) == 0);
	CHECK(rros_rule_to_sway("app_id=mpv border pixel 4", out, sizeof out) == 0);
	CHECK(rros_rule_to_sway("app_id=mpv inhibit_idle", out, sizeof out) == 0);
	CHECK(rros_rule_to_sway("app_id=mpv sticky", out, sizeof out) == 0);
	CHECK(rros_rule_to_sway("app_id=mpv fullscreen", out, sizeof out) == 0);
	CHECK(rros_rule_to_sway("app_id=x workspace 0", out, sizeof out) == -1);
	CHECK(rros_rule_to_sway("app_id=x workspace 33", out, sizeof out) == -1);
	CHECK(rros_rule_to_sway("app_id=x workspace dos", out, sizeof out) == -1);
	CHECK(rros_rule_to_sway("app_id=x opacity 1.5", out, sizeof out) == -1);
	CHECK(rros_rule_to_sway("app_id=x opacity", out, sizeof out) == -1);
	CHECK(rros_rule_to_sway("app_id=x border pixel 99", out, sizeof out) == -1);
	CHECK(rros_rule_to_sway("app_id=x floating extra", out, sizeof out) == -1);
	CHECK(rros_rule_to_sway("color=x floating", out, sizeof out) == -1);       /* criterio no permitido */
	CHECK(rros_rule_to_sway("app_id=a\"b floating", out, sizeof out) == -1);   /* comillas */
	CHECK(rros_rule_to_sway("app_id= floating", out, sizeof out) == -1);
	CHECK(rros_rule_to_sway("firefox floating", out, sizeof out) == -1);

	/* autostart */
	CHECK(rros_autostart_to_sway("syncthing -no-browser", out, sizeof out) == 0);
	CHECK_STR(out, "exec syncthing -no-browser");
	CHECK(rros_autostart_to_sway("", out, sizeof out) == -1);
	CHECK(rros_autostart_to_sway("a\nexec b", out, sizeof out) == -1);
	CHECK(rros_autostart_to_sway("x {", out, sizeof out) == -1);

	/* familias e identificadores */
	CHECK(rros_family_of("keybind.nav") == RROS_FAM_KEYBIND);
	CHECK(rros_family_of("rule.ff") == RROS_FAM_RULE);
	CHECK(rros_family_of("autostart.x") == RROS_FAM_AUTOSTART);
	CHECK(rros_family_of("ui.gap") == RROS_FAM_NONE);
	CHECK(rros_dynamic_check("ui.gap", "5", err, sizeof err) == 1);
	CHECK(rros_dynamic_check("keybind.nav", "Mod+b exec firefox", err, sizeof err) == 0);
	CHECK(rros_dynamic_check("keybind.Nav", "Mod+b exec firefox", err, sizeof err) < 0);   /* mayúsculas */
	CHECK(rros_dynamic_check("keybind.", "Mod+b exec firefox", err, sizeof err) < 0);      /* sin id */
	CHECK(rros_dynamic_check("keybind.a.b", "Mod+b exec firefox", err, sizeof err) < 0);   /* con punto */
	CHECK(rros_dynamic_check("rule.ff", "mal", err, sizeof err) < 0);
	CHECK(strstr(err, "rule.ff") != NULL);
	CHECK(rros_schema_find("panel.style") != NULL);
	CHECK(rros_schema_find("term.scrollback") != NULL);
	DONE();
}
