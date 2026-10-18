#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "check.h"
#include "rros_profile.h"
#include "rros_userconf.h"

static char dir[128];

static void setenv_paths(void)
{
	char buf[200];

	snprintf(buf, sizeof buf, "%s/user.toml", dir);
	setenv("RROS_USER_CONF", buf, 1);
	snprintf(buf, sizeof buf, "%s/system.toml", dir);
	setenv("RROS_SYSTEM_CONF", buf, 1);
}

static const char *val(const char *path, const char *key)
{
	static struct rros_conf c;
	static int used;

	if (used)
		rros_conf_free(&c);
	rros_conf_init(&c);
	used = 1;
	if (rros_conf_load(&c, path, NULL))
		return NULL;
	return rros_conf_get(&c, key);
}

int main(void)
{
	char err[300], user[256], sys[256], cmd[400];
	struct rros_conf c;

	snprintf(dir, sizeof dir, "/tmp/rros-uc-XXXXXX");
	CHECK(mkdtemp(dir) != NULL);
	setenv_paths();
	snprintf(user, sizeof user, "%s/user.toml", dir);
	snprintf(sys, sizeof sys, "%s/system.toml", dir);

	/* set válido: tipos y comillas */
	CHECK(rros_user_set("ui.dock_icon_size", "52", 0, err, sizeof err) == 0);
	CHECK(rros_user_set("ui.theme", "nord", 0, err, sizeof err) == 0);
	CHECK(rros_user_set("ui.blur", "true", 0, err, sizeof err) == 0);
	CHECK(rros_user_set("keybind.nav", "Mod+b exec firefox", 0, err, sizeof err) == 0);
	CHECK_STR(val(user, "ui.dock_icon_size"), "52");
	CHECK_STR(val(user, "ui.theme"), "nord");
	CHECK_STR(val(user, "keybind.nav"), "Mod+b exec firefox");

	/* rechazos con mensaje */
	CHECK(rros_user_set("ui.dock_icon_size", "999", 0, err, sizeof err) < 0);
	CHECK(strstr(err, "fuera de rango") != NULL);
	CHECK(rros_user_set("ui.nope", "1", 0, err, sizeof err) < 0);
	CHECK(strstr(err, "desconocida") != NULL);
	CHECK(rros_user_set("profile.name", "x", 0, err, sizeof err) < 0);
	CHECK(strstr(err, "perfil") != NULL);
	CHECK(rros_user_set("keybind.x", "Mod+b", 0, err, sizeof err) < 0);   /* sin comando */
	CHECK_STR(val(user, "ui.dock_icon_size"), "52"); /* un fallo no toca el archivo */

	/* capa del sistema aparte */
	CHECK(rros_user_set("ui.dock_icon_size", "40", 1, err, sizeof err) == 0);
	CHECK_STR(val(sys, "ui.dock_icon_size"), "40");
	CHECK_STR(val(user, "ui.dock_icon_size"), "52");

	/* unset */
	CHECK(rros_user_unset("ui.dock_icon_size", 0, err, sizeof err) == 0);
	CHECK(val(user, "ui.dock_icon_size") == NULL);
	CHECK(rros_user_unset("ui.nope", 0, err, sizeof err) < 0);

	/* validación de un archivo con claves dinámicas */
	rros_conf_init(&c);
	CHECK(rros_conf_parse(&c, "[ui]\ndock_icon_size = 50\ngap = 5\n[rule]\nok = \"app=x above\"\nbad = \"app=x vuela\"\n[ui]\nzzz = 1\n", NULL) == 0);
	CHECK(rros_conf_validate(&c, 0, NULL, "t") == 2);
	rros_conf_free(&c);

	/* instantáneas */
	CHECK(rros_snapshot_save("antes", err, sizeof err) == 0);
	CHECK(rros_user_set("ui.theme", "gruvbox", 0, err, sizeof err) == 0);
	CHECK(rros_snapshot_save("../malo", err, sizeof err) < 0);   /* nombre inválido */
	CHECK(rros_snapshot_restore("no-existe", err, sizeof err) < 0);
	CHECK(rros_snapshot_restore("antes", err, sizeof err) == 0);
	CHECK_STR(val(user, "ui.theme"), "nord");
	{
		char *buf = NULL;
		size_t len = 0;
		FILE *f = open_memstream(&buf, &len);

		CHECK(rros_snapshot_list(f) == 1);
		fclose(f);
		CHECK_STR(buf, "antes\n");
		free(buf);
	}
	CHECK(rros_snapshot_delete("antes", err, sizeof err) == 0);
	CHECK(rros_snapshot_delete("antes", err, sizeof err) < 0);

	/* import: válido y no válido; deja copia .bak */
	snprintf(cmd, sizeof cmd, "%s/imp.toml", dir);
	{
		FILE *f = fopen(cmd, "w");

		fputs("[ui]\ntheme = \"mono\"\n", f);
		fclose(f);
	}
	CHECK(rros_user_import(cmd, err, sizeof err) == 0);
	CHECK_STR(val(user, "ui.theme"), "mono");
	{
		char bak[300];

		snprintf(bak, sizeof bak, "%s.bak", user);
		CHECK(access(bak, F_OK) == 0);
	}
	{
		FILE *f = fopen(cmd, "w");

		fputs("[ui]\ndock_icon_size = 99999\n", f);
		fclose(f);
	}
	CHECK(rros_user_import(cmd, err, sizeof err) < 0);
	CHECK_STR(val(user, "ui.theme"), "mono"); /* sin cambios */
	CHECK(rros_user_import("/no/existe.toml", err, sizeof err) < 0);

	/* reset */
	CHECK(rros_user_reset(err, sizeof err) == 0);
	CHECK(access(user, F_OK) != 0);

	snprintf(cmd, sizeof cmd, "rm -rf %s", dir);
	if (system(cmd))
		return 3;
	DONE();
}
