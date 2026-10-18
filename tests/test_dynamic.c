#include <string.h>

#include "check.h"
#include "rros_schema.h"

static char combo[96], cmd[256];

static int kb(const char *v)
{
	return rros_keybind_parse(v, combo, sizeof combo, cmd, sizeof cmd);
}

int main(void)
{
	char err[256];
	struct rros_rule r;

	/* atajos: combinación de KDE y comando */
	CHECK(kb("Mod+Shift+b firefox") == 0);
	CHECK_STR(combo, "Meta+Shift+B");
	CHECK_STR(cmd, "firefox");
	CHECK(kb("Mod+Shift+b exec firefox") == 0);       /* «exec» de antes, se quita */
	CHECK_STR(cmd, "firefox");
	CHECK(kb("Shift+Ctrl+Alt+Delete systemctl reboot") == 0);
	CHECK_STR(combo, "Ctrl+Alt+Shift+Delete");          /* en el orden de KDE */
	CHECK_STR(cmd, "systemctl reboot");
	CHECK(kb("Super+Return konsole") == 0);
	CHECK_STR(combo, "Meta+Return");
	CHECK(kb("Print spectacle") == 0);
	CHECK_STR(combo, "Print");
	CHECK(kb("Mod+x") == -1);                    /* sin comando */
	CHECK(kb("Mod+Hyper+x firefox") == -1);      /* modificador desconocido */
	CHECK(kb("Mod++b x") == -1);
	CHECK(kb("Mod+ x") == -1);
	CHECK(kb("Mod+b; x") == -1);
	CHECK(kb("Mod+b {evil}") == -1);
	CHECK(kb("Mod+b x\nExec=y") == -1);          /* sin inyección de líneas en el lanzador */
	CHECK(kb("") == -1);
	CHECK(rros_keybind_parse("Mod+b firefox", combo, 4, cmd, sizeof cmd) == -1);   /* búfer pequeño */

	/* reglas de ventanas de KWin */
	CHECK(rros_rule_parse("app=mpv above", &r) == 0);
	CHECK_STR(r.match, "mpv");
	CHECK_STR(r.action, "above");
	CHECK(r.by_title == 0);
	CHECK(rros_rule_parse("app_id=firefox maximized", &r) == 0);
	CHECK(rros_rule_parse("class=Steam fullscreen", &r) == 0);
	CHECK(rros_rule_parse("title=Video opacity 0.85", &r) == 0);
	CHECK(r.by_title == 1 && r.opacity == 85);
	CHECK(rros_rule_parse("app=mpv opacity 70", &r) == 0);
	CHECK(r.opacity == 70);
	CHECK(rros_rule_parse("app=mpv noborder", &r) == 0);
	CHECK(rros_rule_parse("app=mpv skiptaskbar", &r) == 0);
	CHECK(rros_rule_parse("app=mpv minimized", &r) == 0);
	CHECK(rros_rule_parse("app=x opacity 5", &r) == -1);
	CHECK(rros_rule_parse("app=x opacity", &r) == -1);
	CHECK(rros_rule_parse("app=x above extra", &r) == -1);
	CHECK(rros_rule_parse("app=x workspace 2", &r) == -1);   /* no se puede con KWin */
	CHECK(rros_rule_parse("color=x above", &r) == -1);       /* criterio no permitido */
	CHECK(rros_rule_parse("app=a\"b above", &r) == -1);      /* comillas */
	CHECK(rros_rule_parse("app= above", &r) == -1);
	CHECK(rros_rule_parse("firefox above", &r) == -1);

	/* familias e identificadores */
	CHECK(rros_family_of("keybind.nav") == RROS_FAM_KEYBIND);
	CHECK(rros_family_of("rule.ff") == RROS_FAM_RULE);
	CHECK(rros_family_of("autostart.x") == RROS_FAM_AUTOSTART);
	CHECK(rros_family_of("ui.dock") == RROS_FAM_NONE);
	CHECK(rros_dynamic_check("ui.dock", "true", err, sizeof err) == 1);
	CHECK(rros_dynamic_check("keybind.nav", "Mod+b firefox", err, sizeof err) == 0);
	CHECK(rros_dynamic_check("keybind.Nav", "Mod+b firefox", err, sizeof err) < 0);   /* mayúsculas */
	CHECK(rros_dynamic_check("keybind.", "Mod+b firefox", err, sizeof err) < 0);      /* sin id */
	CHECK(rros_dynamic_check("keybind.a.b", "Mod+b firefox", err, sizeof err) < 0);   /* con punto */
	CHECK(rros_dynamic_check("rule.ff", "mal", err, sizeof err) < 0);
	CHECK(strstr(err, "rule.ff") != NULL);
	CHECK(rros_dynamic_check("autostart.sync", "syncthing -no-browser", err, sizeof err) == 0);
	CHECK(rros_dynamic_check("autostart.x", "a\nb", err, sizeof err) < 0);
	CHECK(rros_schema_find("panel.style") != NULL);
	CHECK(rros_schema_find("ui.dock_style") != NULL);
	CHECK(rros_schema_find("ui.compositor") == NULL);
	DONE();
}
