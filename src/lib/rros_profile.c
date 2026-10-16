#include "rros_profile.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static const char *env_or(const char *name, const char *def)
{
	const char *v = getenv(name);

	return (v && *v) ? v : def;
}

const char *rros_profile_dir(void)
{
	return env_or("RROS_PROFILE_DIR", "/usr/share/rros/profiles");
}

const char *rros_system_conf(void)
{
	return env_or("RROS_SYSTEM_CONF", "/etc/rros/rros.toml");
}

const char *rros_profile_file(void)
{
	return env_or("RROS_PROFILE_FILE", "/etc/rros/profile");
}

int rros_user_conf(char *buf, size_t n)
{
	const char *v = getenv("RROS_USER_CONF");
	const char *xdg = getenv("XDG_CONFIG_HOME");
	const char *home = getenv("HOME");
	int len;

	if (v && *v)
		len = snprintf(buf, n, "%s", v);
	else if (xdg && *xdg)
		len = snprintf(buf, n, "%s/rros/rros.toml", xdg);
	else if (home && *home)
		len = snprintf(buf, n, "%s/.config/rros/rros.toml", home);
	else
		return -1;
	return (len < 0 || (size_t)len >= n) ? -1 : 0;
}

int rros_valid_profile_name(const char *name)
{
	size_t n = strlen(name);

	if (n == 0 || n > 32)
		return 0;
	for (size_t i = 0; i < n; i++) {
		char ch = name[i];

		if (!((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') ||
		      ch == '-' || ch == '_'))
			return 0;
	}
	return 1;
}

int rros_load_profile(struct rros_conf *c, const char *name, int *errline)
{
	char path[1024];
	int rc;

	if (!rros_valid_profile_name(name))
		return -1;
	snprintf(path, sizeof path, "%s/%s.toml", rros_profile_dir(), name);
	rc = rros_conf_load(c, path, errline);
	if (rc == -1)
		return -2;
	return rc == 0 ? 0 : -3;
}

int rros_read_active_profile(char *buf, size_t n)
{
	FILE *f = fopen(rros_profile_file(), "r");

	snprintf(buf, n, "standard");
	if (!f)
		return 0;
	if (fgets(buf, (int)n, f)) {
		buf[strcspn(buf, "\r\n")] = '\0';
		if (!rros_valid_profile_name(buf))
			snprintf(buf, n, "standard");
	}
	fclose(f);
	return 0;
}

int rros_mkdir_parents(const char *path)
{
	char *p = malloc(strlen(path) + 1);
	int rc = 0;

	if (!p)
		return -1;
	strcpy(p, path);
	for (char *s = p + 1; *s; s++) {
		if (*s != '/')
			continue;
		*s = '\0';
		if (mkdir(p, 0755) != 0 && errno != EEXIST) {
			rc = -1;
			break;
		}
		*s = '/';
	}
	free(p);
	return rc;
}

int rros_write_active_profile(const char *name)
{
	const char *path = rros_profile_file();
	char tmp[1100];
	FILE *f;

	if (!rros_valid_profile_name(name))
		return -1;
	if (rros_mkdir_parents(path) != 0)
		return -1;
	snprintf(tmp, sizeof tmp, "%s.tmp", path);
	f = fopen(tmp, "w");
	if (!f)
		return -1;
	fprintf(f, "%s\n", name);
	if (fclose(f) != 0 || rename(tmp, path) != 0) {
		remove(tmp);
		return -1;
	}
	return 0;
}
