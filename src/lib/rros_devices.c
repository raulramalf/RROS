#include "rros_devices.h"

/* Los nombres de /sys se limitan antes de usarlos (strlen > 40 se descarta); GCC no lo ve
 * y avisa de truncados imposibles en las rutas construidas con ellos. */
#pragma GCC diagnostic ignored "-Wformat-truncation"

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const char *const rros_devcats[][2] = {
	{ "disco", "Discos y memorias" }, { "usb", "USB" }, { "red", "Red" }, { "radio", "Radios (Wi-Fi y Bluetooth)" },
	{ "sonido", "Sonido" }, { "pantalla", "Pantallas" }, { "energia", "Energía" }, { "entrada", "Teclado, ratón y mandos" },
	{ "camara", "Cámaras" }, { "tarjeta", "Tarjetas internas (PCI)" },
};
const size_t rros_devcats_len = sizeof rros_devcats / sizeof rros_devcats[0];

/* ---- utilidades ---- */

static void trim(char *s)
{
	size_t n = strlen(s), i = 0;

	while (n && isspace((unsigned char)s[n - 1]))
		s[--n] = '\0';
	while (s[i] == ' ' || s[i] == '\t')
		i++;
	if (i)
		memmove(s, s + i, strlen(s + i) + 1);
}

/* lee root+rel (la primera línea y lo que quepa), sin espacios al final; 0 si existe */
static int rd(const char *root, const char *rel, char *buf, size_t n)
{
	char path[1024];
	FILE *f;
	size_t got;

	snprintf(path, sizeof path, "%s%s", root, rel);
	f = fopen(path, "r");
	if (!f)
		return -1;
	got = fread(buf, 1, n - 1, f);
	buf[got] = '\0';
	fclose(f);
	trim(buf);
	return 0;
}

static int has(const char *root, const char *rel)
{
	char path[1024];

	snprintf(path, sizeof path, "%s%s", root, rel);
	return access(path, F_OK) == 0;
}

static unsigned long long rd_ull(const char *root, const char *rel)
{
	char b[64];

	return rd(root, rel, b, sizeof b) == 0 ? strtoull(b, NULL, 10) : 0;
}

static struct rros_dev *add(struct rros_devlist *l, const char *cat, const char *id)
{
	struct rros_dev *d;

	if (l->n == l->cap) {
		size_t cap = l->cap ? l->cap * 2 : 32;
		struct rros_dev *v = realloc(l->v, cap * sizeof *v);

		if (!v)
			return NULL;
		l->v = v;
		l->cap = cap;
	}
	d = &l->v[l->n++];
	memset(d, 0, sizeof *d);
	snprintf(d->cat, sizeof d->cat, "%s", cat);
	snprintf(d->id, sizeof d->id, "%s", id);
	return d;
}

void rros_devs_free(struct rros_devlist *l)
{
	free(l->v);
	l->v = NULL;
	l->n = l->cap = 0;
}

const struct rros_dev *rros_devs_find(const struct rros_devlist *l, const char *id)
{
	for (size_t i = 0; i < l->n; i++)
		if (!strcmp(l->v[i].id, id))
			return &l->v[i];
	return NULL;
}

void rros_human_size(unsigned long long b, char *buf, size_t n)
{
	if (b >= 1000ULL * 1000 * 1000 * 1000)
		snprintf(buf, n, "%.1f TB", b / 1e12);
	else if (b >= 1000ULL * 1000 * 1000)
		snprintf(buf, n, "%.1f GB", b / 1e9);
	else if (b >= 1000ULL * 1000)
		snprintf(buf, n, "%.0f MB", b / 1e6);
	else
		snprintf(buf, n, "%llu KB", b / 1000);
}

int rros_mount_is_system(const char *mp)
{
	/* udisks monta lo extraíble en /media o /run/media; lo demás lo montó el sistema o un administrador */
	return strncmp(mp, "/media/", 7) && strncmp(mp, "/run/media/", 11);
}

static void cat_str(char *dst, size_t n, const char *sep, const char *s)
{
	if (!s || !*s)
		return;
	if (*dst && strlen(dst) + strlen(sep) < n)
		strcat(dst, sep);
	if (strlen(dst) + strlen(s) < n)
		strcat(dst, s);
}

/* ---- montajes y udev ---- */

struct mnt { char dev[96], dir[160], type[24]; };

static void unescape(char *s)
{
	char *o = s;

	for (; *s; s++) {
		if (s[0] == '\\' && isdigit((unsigned char)s[1]) && isdigit((unsigned char)s[2]) && isdigit((unsigned char)s[3])) {
			*o++ = (char)((s[1] - '0') * 64 + (s[2] - '0') * 8 + (s[3] - '0'));
			s += 3;
		} else {
			*o++ = *s;
		}
	}
	*o = '\0';
}

static int load_mounts(const char *root, struct mnt *m, int max)
{
	char path[1024], line[600];
	FILE *f;
	int n = 0;

	snprintf(path, sizeof path, "%s/proc/self/mounts", root);
	f = fopen(path, "r");
	if (!f)
		return 0;
	while (n < max && fgets(line, sizeof line, f)) {
		if (sscanf(line, "%95s %159s %23s", m[n].dev, m[n].dir, m[n].type) == 3) {
			unescape(m[n].dir);
			n++;
		}
	}
	fclose(f);
	return n;
}

static const char *mounted_at(const struct mnt *m, int n, const char *name)
{
	char dev[64];

	snprintf(dev, sizeof dev, "/dev/%s", name);
	for (int i = 0; i < n; i++)
		if (!strcmp(m[i].dev, dev))
			return m[i].dir;
	return NULL;
}

/* propiedad de la base de datos de udev para un dispositivo de bloque ("8:17") */
static int udev_prop(const char *root, const char *devnum, const char *key, char *out, size_t n)
{
	char path[1024], line[512];
	size_t kl = strlen(key);
	FILE *f;
	int found = 0;

	snprintf(path, sizeof path, "%s/run/udev/data/b%s", root, devnum);
	f = fopen(path, "r");
	if (!f)
		return 0;
	while (!found && fgets(line, sizeof line, f))
		if (!strncmp(line, "E:", 2) && !strncmp(line + 2, key, kl) && line[2 + kl] == '=') {
			snprintf(out, n, "%s", line + 3 + kl);
			trim(out);
			found = 1;
		}
	fclose(f);
	return found;
}

/* ---- discos ---- */

static int skip_block(const char *n)
{
	static const char *pre[] = { ".", "loop", "ram", "zram", "dm-", "nbd", "fd", "md" };

	for (size_t i = 0; i < sizeof pre / sizeof *pre; i++)
		if (!strncmp(n, pre[i], strlen(pre[i])))
			return 1;
	return 0;
}

static void fs_info(const char *root, const char *devnum, char *fstype, size_t fn, char *label, size_t ln)
{
	*fstype = *label = '\0';
	if (*devnum) {
		udev_prop(root, devnum, "ID_FS_TYPE", fstype, fn);
		udev_prop(root, devnum, "ID_FS_LABEL", label, ln);
	}
}

static void volume_actions(struct rros_dev *d, const char *fstype, const char *mp)
{
	if (mp && !rros_mount_is_system(mp))
		snprintf(d->actions, sizeof d->actions, "unmount");
	else if (!mp && *fstype && strcmp(fstype, "swap") && strcmp(fstype, "crypto_LUKS"))
		snprintf(d->actions, sizeof d->actions, "mount");
	if (mp)
		snprintf(d->state, sizeof d->state, "%s %.50s", rros_mount_is_system(mp) ? "sistema:" : "montado en", mp);
	else if (!strcmp(fstype, "swap"))
		snprintf(d->state, sizeof d->state, "intercambio");
	else
		snprintf(d->state, sizeof d->state, *fstype ? "sin montar" : "sin sistema de archivos");
}

static void scan_disks(struct rros_devlist *l, const char *root)
{
	struct mnt m[128];
	int nm = load_mounts(root, m, 128);
	char dir[1024];
	DIR *d;
	struct dirent *e;

	snprintf(dir, sizeof dir, "%s/sys/block", root);
	d = opendir(dir);
	if (!d)
		return;
	while ((e = readdir(d))) {
		const char *n = e->d_name;
		char rel[400], link[512], vendor[64] = "", model[96] = "", size[24], devnum[24] = "", kind[24];
		char fstype[32], label[64], path[1100];
		unsigned long long bytes;
		int sr = !strncmp(n, "sr", 2), removable, usb = 0, nparts = 0;
		struct rros_dev *disk;
		ssize_t ll;
		DIR *pd;
		struct dirent *pe;

		if (skip_block(n) || strlen(n) > 32)
			continue;
		snprintf(rel, sizeof rel, "/sys/block/%s/size", n);
		bytes = rd_ull(root, rel) * 512ULL;
		if (!bytes && !sr)
			continue;   /* lector de tarjetas vacío */
		snprintf(path, sizeof path, "%s/sys/block/%s", root, n);
		ll = readlink(path, link, sizeof link - 1);
		if (ll > 0) {
			link[ll] = '\0';
			usb = strstr(link, "/usb") != NULL;
		}
		snprintf(rel, sizeof rel, "/sys/block/%s/removable", n);
		removable = rd(root, rel, kind, sizeof kind) == 0 && !strcmp(kind, "1");
		snprintf(rel, sizeof rel, "/sys/block/%s/device/vendor", n);
		rd(root, rel, vendor, sizeof vendor);
		snprintf(rel, sizeof rel, "/sys/block/%s/device/model", n);
		rd(root, rel, model, sizeof model);
		snprintf(rel, sizeof rel, "/sys/block/%s/dev", n);
		rd(root, rel, devnum, sizeof devnum);
		snprintf(rel, sizeof rel, "/sys/block/%s/queue/rotational", n);
		if (sr)
			snprintf(kind, sizeof kind, "lector óptico");
		else if (usb)
			snprintf(kind, sizeof kind, "USB");
		else if (!strncmp(n, "nvme", 4))
			snprintf(kind, sizeof kind, "NVMe");
		else if (!strncmp(n, "mmcblk", 6))
			snprintf(kind, sizeof kind, "tarjeta SD");
		else if (rd(root, rel, kind, sizeof kind) == 0 && !strcmp(kind, "0"))
			snprintf(kind, sizeof kind, "SSD");
		else
			snprintf(kind, sizeof kind, "disco duro");

		disk = add(l, "disco", n);
		if (!disk)
			break;
		if (!strncmp(vendor, "0x", 2))
			*vendor = '\0';   /* los discos virtio dan el código del fabricante, no el nombre */
		cat_str(disk->name, sizeof disk->name, " ", vendor);
		cat_str(disk->name, sizeof disk->name, " ", model);
		if (!*disk->name)
			snprintf(disk->name, sizeof disk->name, "%s", !strncmp(n, "vd", 2) ? "Disco virtual" : n);
		rros_human_size(bytes, size, sizeof size);
		snprintf(disk->detail, sizeof disk->detail, "%s · %s", bytes ? size : "sin disco", kind);
		if (removable || usb || sr)
			snprintf(disk->actions, sizeof disk->actions, "eject");

		/* particiones: subdirectorios con un archivo "partition" */
		pd = opendir(path);
		while (pd && (pe = readdir(pd))) {
			char pdev[24] = "", psize[24];
			const char *mp;
			struct rros_dev *p;

			if (strncmp(pe->d_name, n, strlen(n)) || strlen(pe->d_name) > 40)
				continue;
			snprintf(rel, sizeof rel, "/sys/block/%s/%s/partition", n, pe->d_name);
			if (!has(root, rel))
				continue;
			nparts++;
			snprintf(rel, sizeof rel, "/sys/block/%s/%s/dev", n, pe->d_name);
			rd(root, rel, pdev, sizeof pdev);
			snprintf(rel, sizeof rel, "/sys/block/%s/%s/size", n, pe->d_name);
			rros_human_size(rd_ull(root, rel) * 512ULL, psize, sizeof psize);
			fs_info(root, pdev, fstype, sizeof fstype, label, sizeof label);
			mp = mounted_at(m, nm, pe->d_name);
			p = add(l, "disco", pe->d_name);
			if (!p)
				break;
			disk = NULL;   /* add() puede mover la lista */
			snprintf(p->parent, sizeof p->parent, "%s", n);
			snprintf(p->name, sizeof p->name, "%s", *label ? label : pe->d_name);
			snprintf(p->detail, sizeof p->detail, "%s%s%s", psize, *fstype ? " · " : "", fstype);
			volume_actions(p, fstype, mp);
		}
		if (pd)
			closedir(pd);
		/* disco sin particiones con sistema de archivos (CD, algunas memorias) */
		if (!nparts) {
			struct rros_dev *dd = &l->v[l->n - 1];
			const char *mp = mounted_at(m, nm, n);

			fs_info(root, devnum, fstype, sizeof fstype, label, sizeof label);
			if (*fstype || mp) {
				char acts[48];

				snprintf(acts, sizeof acts, "%s", dd->actions);
				volume_actions(dd, fstype, mp);
				if (*acts && strlen(dd->actions) + strlen(acts) + 2 < sizeof dd->actions)
					cat_str(dd->actions, sizeof dd->actions, ",", acts);
				if (*label)
					cat_str(dd->detail, sizeof dd->detail, " · ", label);
			} else if (sr) {
				snprintf(dd->state, sizeof dd->state, "vacío");
			}
		}
	}
	closedir(d);
}

/* ---- USB ---- */

static void scan_usb(struct rros_devlist *l, const char *root)
{
	char dir[1024];
	DIR *d;
	struct dirent *e;

	snprintf(dir, sizeof dir, "%s/sys/bus/usb/devices", root);
	d = opendir(dir);
	if (!d)
		return;
	while ((e = readdir(d))) {
		char rel[400], vid[16] = "", pid[16] = "", cls[8] = "", man[96] = "", prod[96] = "", speed[16] = "";
		struct rros_dev *u;

		if (e->d_name[0] == '.' || strchr(e->d_name, ':') || strlen(e->d_name) > 40)
			continue;
		snprintf(rel, sizeof rel, "/sys/bus/usb/devices/%s/idVendor", e->d_name);
		if (rd(root, rel, vid, sizeof vid))
			continue;
		snprintf(rel, sizeof rel, "/sys/bus/usb/devices/%s/bDeviceClass", e->d_name);
		if (rd(root, rel, cls, sizeof cls) == 0 && !strcmp(cls, "09"))
			continue;   /* concentradores */
		snprintf(rel, sizeof rel, "/sys/bus/usb/devices/%s/idProduct", e->d_name);
		rd(root, rel, pid, sizeof pid);
		snprintf(rel, sizeof rel, "/sys/bus/usb/devices/%s/manufacturer", e->d_name);
		rd(root, rel, man, sizeof man);
		snprintf(rel, sizeof rel, "/sys/bus/usb/devices/%s/product", e->d_name);
		rd(root, rel, prod, sizeof prod);
		snprintf(rel, sizeof rel, "/sys/bus/usb/devices/%s/speed", e->d_name);
		rd(root, rel, speed, sizeof speed);
		u = add(l, "usb", e->d_name);
		if (!u)
			break;
		cat_str(u->name, sizeof u->name, " ", man);
		cat_str(u->name, sizeof u->name, " ", prod);
		if (!*u->name)
			snprintf(u->name, sizeof u->name, "Dispositivo USB");
		snprintf(u->detail, sizeof u->detail, "%s:%s%s%s%s", vid, pid, *speed ? " · " : "", speed, *speed ? " Mb/s" : "");
		snprintf(u->state, sizeof u->state, "conectado");
	}
	closedir(d);
}

/* ---- red ---- */

static void scan_net(struct rros_devlist *l, const char *root)
{
	char dir[1024];
	DIR *d;
	struct dirent *e;

	snprintf(dir, sizeof dir, "%s/sys/class/net", root);
	d = opendir(dir);
	if (!d)
		return;
	while ((e = readdir(d))) {
		char rel[400], oper[24] = "", mac[32] = "", speed[16] = "";
		int wifi;
		struct rros_dev *n;

		if (e->d_name[0] == '.' || !strcmp(e->d_name, "lo") || strlen(e->d_name) > 40)
			continue;
		snprintf(rel, sizeof rel, "/sys/class/net/%s/device", e->d_name);
		if (!has(root, rel))
			continue;   /* virtuales: puentes, VPN, contenedores */
		snprintf(rel, sizeof rel, "/sys/class/net/%s/wireless", e->d_name);
		wifi = has(root, rel);
		snprintf(rel, sizeof rel, "/sys/class/net/%s/operstate", e->d_name);
		rd(root, rel, oper, sizeof oper);
		snprintf(rel, sizeof rel, "/sys/class/net/%s/address", e->d_name);
		rd(root, rel, mac, sizeof mac);
		snprintf(rel, sizeof rel, "/sys/class/net/%s/speed", e->d_name);
		if (!wifi && !strcmp(oper, "up"))
			rd(root, rel, speed, sizeof speed);
		n = add(l, "red", e->d_name);
		if (!n)
			break;
		snprintf(n->name, sizeof n->name, "%s (%s)", wifi ? "Wi-Fi" : "Ethernet", e->d_name);
		snprintf(n->detail, sizeof n->detail, "%s%s%s%s", mac, *speed && speed[0] != '-' ? " · " : "",
			 *speed && speed[0] != '-' ? speed : "", *speed && speed[0] != '-' ? " Mb/s" : "");
		snprintf(n->state, sizeof n->state, "%s", !strcmp(oper, "up") ? "conectado" :
			 !strcmp(oper, "down") ? (wifi ? "desconectado" : "sin cable") : *oper ? oper : "desconocido");
	}
	closedir(d);
}

/* ---- radios (rfkill) ---- */

static void scan_rfkill(struct rros_devlist *l, const char *root)
{
	char dir[1024];
	DIR *d;
	struct dirent *e;

	snprintf(dir, sizeof dir, "%s/sys/class/rfkill", root);
	d = opendir(dir);
	if (!d)
		return;
	while ((e = readdir(d))) {
		char rel[400], type[24] = "", name[64] = "", soft[8] = "0", hard[8] = "0";
		struct rros_dev *r;

		if (strncmp(e->d_name, "rfkill", 6) || strlen(e->d_name) > 40)
			continue;
		snprintf(rel, sizeof rel, "/sys/class/rfkill/%s/type", e->d_name);
		rd(root, rel, type, sizeof type);
		snprintf(rel, sizeof rel, "/sys/class/rfkill/%s/name", e->d_name);
		rd(root, rel, name, sizeof name);
		snprintf(rel, sizeof rel, "/sys/class/rfkill/%s/soft", e->d_name);
		rd(root, rel, soft, sizeof soft);
		snprintf(rel, sizeof rel, "/sys/class/rfkill/%s/hard", e->d_name);
		rd(root, rel, hard, sizeof hard);
		r = add(l, "radio", e->d_name);
		if (!r)
			break;
		snprintf(r->name, sizeof r->name, "%s", !strcmp(type, "wlan") ? "Wi-Fi" : !strcmp(type, "bluetooth") ? "Bluetooth" :
			 !strcmp(type, "wwan") ? "Datos móviles" : *type ? type : "Radio");
		snprintf(r->detail, sizeof r->detail, "%s", name);
		if (!strcmp(hard, "1"))
			snprintf(r->state, sizeof r->state, "apagado por el interruptor del equipo");
		else if (!strcmp(soft, "1")) {
			snprintf(r->state, sizeof r->state, "desactivado");
			snprintf(r->actions, sizeof r->actions, "unblock");
		} else {
			snprintf(r->state, sizeof r->state, "activo");
			snprintf(r->actions, sizeof r->actions, "block");
		}
	}
	closedir(d);
}

/* ---- sonido ---- */

static void scan_sound(struct rros_devlist *l, const char *root)
{
	char path[1024], line[300];
	FILE *f;

	snprintf(path, sizeof path, "%s/proc/asound/cards", root);
	f = fopen(path, "r");
	if (!f)
		return;
	while (fgets(line, sizeof line, f)) {
		int num;
		char *dash = strstr(line, " - "), id[16];
		struct rros_dev *s;

		if (sscanf(line, " %d [", &num) != 1 || !dash)
			continue;
		snprintf(id, sizeof id, "card%d", num);
		s = add(l, "sonido", id);
		if (!s)
			break;
		snprintf(s->name, sizeof s->name, "%s", dash + 3);
		trim(s->name);
		snprintf(s->state, sizeof s->state, "disponible");
	}
	fclose(f);
}

/* ---- pantallas ---- */

static void scan_displays(struct rros_devlist *l, const char *root)
{
	char dir[1024];
	DIR *d;
	struct dirent *e;

	snprintf(dir, sizeof dir, "%s/sys/class/drm", root);
	d = opendir(dir);
	if (!d)
		return;
	while ((e = readdir(d))) {
		char rel[400], status[24] = "", mode[32] = "";
		const char *dash = strchr(e->d_name, '-');
		struct rros_dev *p;

		if (strncmp(e->d_name, "card", 4) || !dash || strstr(e->d_name, "Writeback") || strlen(e->d_name) > 40)
			continue;
		snprintf(rel, sizeof rel, "/sys/class/drm/%s/status", e->d_name);
		if (rd(root, rel, status, sizeof status) || strcmp(status, "connected"))
			continue;
		snprintf(rel, sizeof rel, "/sys/class/drm/%s/modes", e->d_name);
		rd(root, rel, mode, sizeof mode);
		if (strchr(mode, '\n'))
			*strchr(mode, '\n') = '\0';
		p = add(l, "pantalla", e->d_name);
		if (!p)
			break;
		snprintf(p->name, sizeof p->name, "%s", dash + 1);
		snprintf(p->detail, sizeof p->detail, "%s%s", *mode ? "máxima " : "", mode);
		snprintf(p->state, sizeof p->state, "conectada");
	}
	closedir(d);
}

/* ---- energía ---- */

static void scan_power(struct rros_devlist *l, const char *root)
{
	char dir[1024];
	DIR *d;
	struct dirent *e;

	snprintf(dir, sizeof dir, "%s/sys/class/power_supply", root);
	d = opendir(dir);
	if (!d)
		return;
	while ((e = readdir(d))) {
		char rel[400], type[24] = "", cap[8] = "", st[24] = "", model[64] = "", online[8] = "";
		struct rros_dev *p;

		if (e->d_name[0] == '.' || strlen(e->d_name) > 40)
			continue;
		snprintf(rel, sizeof rel, "/sys/class/power_supply/%s/type", e->d_name);
		if (rd(root, rel, type, sizeof type))
			continue;
		if (!strcmp(type, "Battery")) {
			unsigned long long full, design;

			snprintf(rel, sizeof rel, "/sys/class/power_supply/%s/capacity", e->d_name);
			rd(root, rel, cap, sizeof cap);
			snprintf(rel, sizeof rel, "/sys/class/power_supply/%s/status", e->d_name);
			rd(root, rel, st, sizeof st);
			snprintf(rel, sizeof rel, "/sys/class/power_supply/%s/model_name", e->d_name);
			rd(root, rel, model, sizeof model);
			snprintf(rel, sizeof rel, "/sys/class/power_supply/%s/energy_full", e->d_name);
			full = rd_ull(root, rel);
			snprintf(rel, sizeof rel, "/sys/class/power_supply/%s/energy_full_design", e->d_name);
			design = rd_ull(root, rel);
			p = add(l, "energia", e->d_name);
			if (!p)
				break;
			snprintf(p->name, sizeof p->name, "Batería%s%s", *model ? " " : "", model);
			if (design)
				snprintf(p->detail, sizeof p->detail, "%s%% · salud %llu%%", *cap ? cap : "?", full * 100 / design);
			else
				snprintf(p->detail, sizeof p->detail, "%s%%", *cap ? cap : "?");
			snprintf(p->state, sizeof p->state, "%s", !strcmp(st, "Charging") ? "cargando" :
				 !strcmp(st, "Discharging") ? "descargando" : !strcmp(st, "Full") ? "cargada" :
				 !strcmp(st, "Not charging") ? "enchufada, sin cargar" : *st ? st : "desconocido");
		} else if (!strcmp(type, "Mains")) {
			snprintf(rel, sizeof rel, "/sys/class/power_supply/%s/online", e->d_name);
			rd(root, rel, online, sizeof online);
			p = add(l, "energia", e->d_name);
			if (!p)
				break;
			snprintf(p->name, sizeof p->name, "Cargador");
			snprintf(p->state, sizeof p->state, "%s", !strcmp(online, "1") ? "enchufado" : "sin enchufar");
		}
	}
	closedir(d);
}

/* ---- entrada ---- */

static void scan_input(struct rros_devlist *l, const char *root)
{
	char path[1024], line[512], name[128] = "", handlers[256] = "";
	FILE *f;
	int idx = 0;

	snprintf(path, sizeof path, "%s/proc/bus/input/devices", root);
	f = fopen(path, "r");
	if (!f)
		return;
	for (;;) {
		char *got = fgets(line, sizeof line, f);

		if (!got || line[0] == '\n') {
			const char *kind = NULL;

			if (*name) {
				int touch = strstr(name, "ouchpad") || strstr(name, "ouchPad");

				if (strstr(handlers, "mouse"))
					kind = touch ? "Touchpad" : "Ratón";
				else if (strstr(handlers, "js"))
					kind = "Mando";
				else if (strstr(handlers, "kbd") && (strstr(name, "eyboard") || strstr(name, "AT Translated")))
					kind = "Teclado";
			}
			if (kind) {
				char id[24];
				int dup = 0;
				struct rros_dev *in;

				for (size_t i = 0; i < l->n; i++)
					if (!strcmp(l->v[i].cat, "entrada") && !strcmp(l->v[i].detail, name))
						dup = 1;
				snprintf(id, sizeof id, "input%d", idx);
				if (!dup && (in = add(l, "entrada", id))) {
					snprintf(in->name, sizeof in->name, "%s", kind);
					snprintf(in->detail, sizeof in->detail, "%s", name);
					snprintf(in->state, sizeof in->state, "conectado");
				}
			}
			*name = *handlers = '\0';
			idx++;
			if (!got)
				break;
			continue;
		}
		if (!strncmp(line, "N: Name=\"", 9)) {
			snprintf(name, sizeof name, "%.120s", line + 9);
			if (strrchr(name, '"'))
				*strrchr(name, '"') = '\0';
		} else if (!strncmp(line, "H: Handlers=", 12)) {
			snprintf(handlers, sizeof handlers, "%.250s", line + 12);
		}
	}
	fclose(f);
}

/* ---- cámaras ---- */

static void scan_cameras(struct rros_devlist *l, const char *root)
{
	char dir[1024];
	DIR *d;
	struct dirent *e;

	snprintf(dir, sizeof dir, "%s/sys/class/video4linux", root);
	d = opendir(dir);
	if (!d)
		return;
	while ((e = readdir(d))) {
		char rel[400], name[96] = "", index[8] = "0";
		struct rros_dev *c;

		if (strncmp(e->d_name, "video", 5) || strlen(e->d_name) > 40)
			continue;
		snprintf(rel, sizeof rel, "/sys/class/video4linux/%s/index", e->d_name);
		rd(root, rel, index, sizeof index);
		if (strcmp(index, "0"))
			continue;   /* cada cámara crea varios nodos: solo el principal */
		snprintf(rel, sizeof rel, "/sys/class/video4linux/%s/name", e->d_name);
		rd(root, rel, name, sizeof name);
		c = add(l, "camara", e->d_name);
		if (!c)
			break;
		snprintf(c->name, sizeof c->name, "%s", *name ? name : "Cámara");
		snprintf(c->state, sizeof c->state, "disponible");
	}
	closedir(d);
}

/* ---- tarjetas PCI ---- */

static const char *pci_kind(unsigned cls)
{
	switch (cls >> 8) {
	case 0x0300: case 0x0302: case 0x0380: return "Gráfica";
	case 0x0200: return "Red por cable";
	case 0x0280: return "Red inalámbrica";
	case 0x0401: case 0x0403: return "Audio";
	case 0x0c03: return "Controladora USB";
	case 0x0106: return "Controladora SATA";
	case 0x0108: return "Controladora NVMe";
	case 0x0d11: return "Bluetooth";
	default: return NULL;
	}
}

/* nombre en pci.ids: "Intel Corporation · Wi-Fi 6 AX201" */
static void pci_name(const char *root, unsigned ven, unsigned dev, char *out, size_t n)
{
	static const char *files[] = { "/usr/share/misc/pci.ids", "/usr/share/hwdata/pci.ids" };
	char line[512], vname[160] = "";
	FILE *f = NULL;
	int in_vendor = 0;

	for (size_t i = 0; i < 2 && !f; i++) {
		char path[1024];

		snprintf(path, sizeof path, "%s%s", root, files[i]);
		f = fopen(path, "r");
	}
	snprintf(out, n, "%04x:%04x", ven, dev);
	if (!f)
		return;
	while (fgets(line, sizeof line, f)) {
		unsigned id;

		if (line[0] == '#' || line[0] == '\n')
			continue;
		if (isxdigit((unsigned char)line[0])) {
			if (in_vendor)
				break;   /* pasamos al siguiente fabricante sin encontrar el modelo */
			if (sscanf(line, "%4x", &id) == 1 && id == ven) {
				in_vendor = 1;
				snprintf(vname, sizeof vname, "%.150s", line + 6);
				trim(vname);
				snprintf(out, n, "%s", vname);
			}
		} else if (in_vendor && line[0] == '\t' && line[1] != '\t' && sscanf(line + 1, "%4x", &id) == 1 && id == dev) {
			char dname[200];

			snprintf(dname, sizeof dname, "%.190s", line + 7);
			trim(dname);
			snprintf(out, n, "%.60s · %.60s", vname, dname);
			break;
		}
	}
	fclose(f);
}

static void scan_pci(struct rros_devlist *l, const char *root)
{
	char dir[1024];
	DIR *d;
	struct dirent *e;

	snprintf(dir, sizeof dir, "%s/sys/bus/pci/devices", root);
	d = opendir(dir);
	if (!d)
		return;
	while ((e = readdir(d))) {
		char rel[400], b[32];
		unsigned cls = 0, ven = 0, dev = 0;
		const char *kind;
		struct rros_dev *p;

		if (e->d_name[0] == '.' || strlen(e->d_name) > 40)
			continue;
		snprintf(rel, sizeof rel, "/sys/bus/pci/devices/%s/class", e->d_name);
		if (rd(root, rel, b, sizeof b) == 0)
			cls = (unsigned)strtoul(b, NULL, 16);
		if (!(kind = pci_kind(cls)))
			continue;
		snprintf(rel, sizeof rel, "/sys/bus/pci/devices/%s/vendor", e->d_name);
		if (rd(root, rel, b, sizeof b) == 0)
			ven = (unsigned)strtoul(b, NULL, 16);
		snprintf(rel, sizeof rel, "/sys/bus/pci/devices/%s/device", e->d_name);
		if (rd(root, rel, b, sizeof b) == 0)
			dev = (unsigned)strtoul(b, NULL, 16);
		p = add(l, "tarjeta", e->d_name);
		if (!p)
			break;
		snprintf(p->name, sizeof p->name, "%s", kind);
		pci_name(root, ven, dev, p->detail, sizeof p->detail);
		snprintf(rel, sizeof rel, "/sys/bus/pci/devices/%s/driver", e->d_name);
		snprintf(p->state, sizeof p->state, "%s", has(root, rel) ? "con controlador" : "sin controlador");
	}
	closedir(d);
}

static int by_id(const void *a, const void *b)
{
	const struct rros_dev *x = a, *y = b;

	return strcmp(x->id, y->id);
}

void rros_devs_scan(struct rros_devlist *l, const char *root)
{
	size_t start;

	memset(l, 0, sizeof *l);
	if (!root)
		root = "";
	scan_disks(l, root);
	/* orden estable dentro de cada categoría (readdir no lo garantiza) */
	if (l->n)
		qsort(l->v, l->n, sizeof *l->v, by_id);
#define SORTED(fn) do { start = l->n; fn(l, root); if (l->n > start) \
	qsort(l->v + start, l->n - start, sizeof *l->v, by_id); } while (0)
	SORTED(scan_usb);
	SORTED(scan_net);
	SORTED(scan_rfkill);
	SORTED(scan_sound);
	SORTED(scan_displays);
	SORTED(scan_power);
	scan_input(l, root);
	SORTED(scan_cameras);
	SORTED(scan_pci);
#undef SORTED
}
