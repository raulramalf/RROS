#include "rros_disks.h"

/* Los nombres de /sys se limitan antes de usarlos (strlen > 32 se descarta); GCC no lo ve
 * y avisa de truncados imposibles en las rutas construidas con ellos. */
#pragma GCC diagnostic ignored "-Wformat-truncation"

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "rros_devices.h"

#define GUID_ESP       "c12a7328-f81f-11d2-ba4b-00a0c93ec93b"
#define GUID_BIOS_BOOT "21686148-6449-6e6f-744e-656564454649"
#define GUID_MSR       "e3c9e316-0b5c-4db8-817d-f92df00215ae"
#define GUID_WIN_RECOVERY "de94bba4-06d1-4d40-a16a-bfd50179d6ac"
#define GUID_LINUX     "0fc63daf-8483-4772-8e79-3d69d8477de4"
#define GUID_ROOT_X64  "4f68bce3-e8cd-4db1-96e7-fbcaf984b709"

static unsigned long long rd_ull(const char *root, const char *rel)
{
	char b[64];

	return rros_sys_read(root, rel, b, sizeof b) == 0 ? strtoull(b, NULL, 10) : 0;
}

static void lower(char *s)
{
	for (; *s; s++)
		*s = (char)tolower((unsigned char)*s);
}

static int skip_block(const char *n)
{
	static const char *pre[] = { ".", "loop", "ram", "zram", "dm-", "nbd", "fd", "md", "sr" };

	for (size_t i = 0; i < sizeof pre / sizeof *pre; i++)
		if (!strncmp(n, pre[i], strlen(pre[i])))
			return 1;
	return 0;
}

static int in_list(const char *s, const char *const *v)
{
	for (; *v; v++)
		if (!strcmp(s, *v))
			return 1;
	return 0;
}

static void classify(struct rros_disk *d, struct rros_dpart *p)
{
	static const char *const linux_fs[] = { "ext4", "ext3", "ext2", "btrfs", "xfs", "f2fs", "crypto_LUKS", NULL };
	static const char *const linux_pt[] = { GUID_LINUX, GUID_ROOT_X64, "0x83", NULL };

	p->esp = !strcmp(p->ptype, GUID_ESP) || !strcmp(p->ptype, "0xef");
	p->bios_boot = !strcmp(p->ptype, GUID_BIOS_BOOT);
	p->windows = !strcmp(p->ptype, GUID_MSR) || !strcmp(p->ptype, GUID_WIN_RECOVERY) ||
		     !strcmp(p->fstype, "ntfs") || !strcmp(p->fstype, "BitLocker");
	p->linux_os = in_list(p->fstype, linux_fs) && (in_list(p->ptype, linux_pt) || !*p->ptype);
	if (p->esp && d->esp < 0)
		d->esp = (int)(p - d->parts);
	d->bios_boot |= p->bios_boot;
	d->windows |= p->windows;
	d->bitlocker |= !strcmp(p->fstype, "BitLocker");
	d->linux_os |= p->linux_os;
}

static int by_start(const void *a, const void *b)
{
	const struct rros_dpart *x = a, *y = b;

	return x->start < y->start ? -1 : x->start > y->start;
}

static unsigned long long align_up(unsigned long long v)
{
	return (v + RROS_MIB - 1) / RROS_MIB * RROS_MIB;
}

static void gap(struct rros_disk *d, unsigned long long a, unsigned long long b)
{
	a = align_up(a);
	b = b / RROS_MIB * RROS_MIB;
	if (b > a && b - a > d->free_size) {
		d->free_start = a;
		d->free_size = b - a;
	}
}

void rros_disk_free(struct rros_disk *d)
{
	struct rros_dpart sorted[32];
	/* 1 MiB al principio (tabla y alineación) y 1 MiB al final (copia de la tabla GPT) */
	unsigned long long cur = RROS_MIB, hi = d->size > 2 * RROS_MIB ? d->size - RROS_MIB : 0;
	int n = d->nparts < 32 ? d->nparts : 32;

	d->free_start = d->free_size = 0;
	if (n > 0)
		memcpy(sorted, d->parts, (size_t)n * sizeof *sorted);
	if (n > 1)
		qsort(sorted, (size_t)n, sizeof *sorted, by_start);
	for (int i = 0; i < n; i++) {
		unsigned long long end = sorted[i].start + sorted[i].size;

		if (sorted[i].start > cur)
			gap(d, cur, sorted[i].start < hi ? sorted[i].start : hi);
		if (end > cur)
			cur = end;
	}
	if (hi > cur)
		gap(d, cur, hi);
}

void rros_disk_systems(const struct rros_disk *d, char *out, size_t n)
{
	snprintf(out, n, "%s%s%s%s", d->windows ? "Windows" : "", d->bitlocker ? " (BitLocker)" : "",
		 d->windows && d->linux_os ? ", " : "", d->linux_os ? "Linux" : "");
}

/* partición activada como intercambio (el live de Ubuntu activa las que encuentra) */
static int is_swap_on(const char *root, const char *name)
{
	char path[1024], line[400], dev[64];
	FILE *f;
	int on = 0;

	snprintf(path, sizeof path, "%s/proc/swaps", root);
	snprintf(dev, sizeof dev, "/dev/%s", name);
	f = fopen(path, "r");
	if (!f)
		return 0;
	while (!on && fgets(line, sizeof line, f))
		on = !strncmp(line, dev, strlen(dev)) && isspace((unsigned char)line[strlen(dev)]);
	fclose(f);
	return on;
}

/* otro programa la tiene abierta: LUKS abierto, LVM, RAID... (/sys/block/X/Y/holders no vacío) */
static int has_holders(const char *root, const char *rel)
{
	char path[1024];
	DIR *d;
	struct dirent *e;
	int any = 0;

	snprintf(path, sizeof path, "%s%s/holders", root, rel);
	d = opendir(path);
	if (!d)
		return 0;
	while (!any && (e = readdir(d)))
		any = e->d_name[0] != '.';
	closedir(d);
	return any;
}

static void mark_busy(struct rros_disk *d, const char *name, const char *mount, int swap, int held)
{
	if (*d->in_use)
		return;
	if (*mount && rros_mount_is_system(mount))
		snprintf(d->in_use, sizeof d->in_use, "lo usa el sistema en marcha (%.60s)", mount);
	else if (*mount)
		snprintf(d->in_use, sizeof d->in_use, "%.30s está montada en %.60s: desmóntala antes", name, mount);
	else if (swap)
		snprintf(d->in_use, sizeof d->in_use, "%.30s se usa como intercambio: sudo swapoff -a", name);
	else if (held)
		snprintf(d->in_use, sizeof d->in_use, "%.30s está en uso (cifrado o LVM abierto)", name);
}

static void scan_one(struct rros_disk *d, const char *root, const char *n)
{
	char rel[400], path[1100], link[512], vendor[64] = "", model[96] = "", devnum[24] = "", b[24];
	ssize_t ll;
	int usb = 0;
	DIR *pd;
	struct dirent *pe;

	memset(d, 0, sizeof *d);
	d->esp = -1;
	snprintf(d->name, sizeof d->name, "%s", n);
	snprintf(rel, sizeof rel, "/sys/block/%s/size", n);
	d->size = rd_ull(root, rel) * 512ULL;
	snprintf(path, sizeof path, "%s/sys/block/%s", root, n);
	ll = readlink(path, link, sizeof link - 1);
	if (ll > 0) {
		link[ll] = '\0';
		usb = strstr(link, "/usb") != NULL;
	}
	snprintf(rel, sizeof rel, "/sys/block/%s/device/vendor", n);
	rros_sys_read(root, rel, vendor, sizeof vendor);
	snprintf(rel, sizeof rel, "/sys/block/%s/device/model", n);
	rros_sys_read(root, rel, model, sizeof model);
	if (!strncmp(vendor, "0x", 2))
		*vendor = '\0';
	snprintf(d->model, sizeof d->model, "%s%s%s", vendor, *vendor && *model ? " " : "", model);
	if (!*d->model)
		snprintf(d->model, sizeof d->model, "%s", !strncmp(n, "vd", 2) ? "Disco virtual" : n);
	snprintf(rel, sizeof rel, "/sys/block/%s/queue/rotational", n);
	if (usb)
		snprintf(d->kind, sizeof d->kind, "USB");
	else if (!strncmp(n, "nvme", 4))
		snprintf(d->kind, sizeof d->kind, "NVMe");
	else if (!strncmp(n, "mmcblk", 6))
		snprintf(d->kind, sizeof d->kind, "tarjeta SD");
	else if (rros_sys_read(root, rel, b, sizeof b) == 0 && !strcmp(b, "0"))
		snprintf(d->kind, sizeof d->kind, "SSD");
	else
		snprintf(d->kind, sizeof d->kind, "disco duro");
	snprintf(rel, sizeof rel, "/sys/block/%s/dev", n);
	rros_sys_read(root, rel, devnum, sizeof devnum);
	if (*devnum)
		rros_udev_prop(root, devnum, "ID_PART_TABLE_TYPE", d->pttype, sizeof d->pttype);

	pd = opendir(path);
	while (pd && (pe = readdir(pd)) && d->nparts < 32) {
		struct rros_dpart *p = &d->parts[d->nparts];
		char pdev[24] = "";

		if (strncmp(pe->d_name, n, strlen(n)) || strlen(pe->d_name) >= sizeof p->name)
			continue;
		snprintf(rel, sizeof rel, "/sys/block/%s/%s/partition", n, pe->d_name);
		if (rros_sys_read(root, rel, b, sizeof b) != 0)
			continue;
		memset(p, 0, sizeof *p);
		snprintf(p->name, sizeof p->name, "%s", pe->d_name);
		snprintf(rel, sizeof rel, "/sys/block/%s/%s/start", n, pe->d_name);
		p->start = rd_ull(root, rel) * 512ULL;
		snprintf(rel, sizeof rel, "/sys/block/%s/%s/size", n, pe->d_name);
		p->size = rd_ull(root, rel) * 512ULL;
		snprintf(rel, sizeof rel, "/sys/block/%s/%s/dev", n, pe->d_name);
		rros_sys_read(root, rel, pdev, sizeof pdev);
		if (*pdev) {
			rros_udev_prop(root, pdev, "ID_FS_TYPE", p->fstype, sizeof p->fstype);
			rros_udev_prop(root, pdev, "ID_FS_LABEL", p->label, sizeof p->label);
			rros_udev_prop(root, pdev, "ID_PART_ENTRY_TYPE", p->ptype, sizeof p->ptype);
			lower(p->ptype);
		}
		rros_mount_of(root, p->name, p->mount, sizeof p->mount);
		snprintf(rel, sizeof rel, "/sys/block/%s/%s", n, pe->d_name);
		mark_busy(d, p->name, p->mount, is_swap_on(root, p->name), has_holders(root, rel));
		classify(d, p);
		d->nparts++;
	}
	if (pd)
		closedir(pd);
	{
		char mp[96] = "";

		rros_mount_of(root, n, mp, sizeof mp);
		snprintf(rel, sizeof rel, "/sys/block/%s", n);
		mark_busy(d, n, mp, is_swap_on(root, n), has_holders(root, rel));
	}
	rros_disk_free(d);
}

static int by_name(const void *a, const void *b)
{
	return strcmp(((const struct rros_disk *)a)->name, ((const struct rros_disk *)b)->name);
}

int rros_disks_scan(struct rros_disk *v, int max, const char *root)
{
	char dir[1024], rel[400];
	DIR *d;
	struct dirent *e;
	int n = 0;

	snprintf(dir, sizeof dir, "%s/sys/block", root);
	d = opendir(dir);
	if (!d)
		return 0;
	while (n < max && (e = readdir(d))) {
		if (skip_block(e->d_name) || strlen(e->d_name) > 32)
			continue;
		snprintf(rel, sizeof rel, "/sys/block/%s/size", e->d_name);
		if (rd_ull(root, rel) == 0)
			continue;   /* lector de tarjetas vacío */
		scan_one(&v[n++], root, e->d_name);
	}
	closedir(d);
	if (n > 1)
		qsort(v, (size_t)n, sizeof *v, by_name);
	return n;
}
