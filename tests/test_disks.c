/* Discos para el instalador sobre un /sys, /proc y /run falsos. */
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "check.h"
#include "rros_disks.h"

static char root[256];

static void put(const char *rel, const char *text)
{
	char path[600], *slash;

	snprintf(path, sizeof path, "%s%s", root, rel);
	for (slash = strchr(path + strlen(root) + 1, '/'); slash; slash = strchr(slash + 1, '/')) {
		*slash = '\0';
		mkdir(path, 0755);
		*slash = '/';
	}
	FILE *f = fopen(path, "w");

	if (f) {
		fputs(text, f);
		fclose(f);
	}
}

static void link_to(const char *rel, const char *target)
{
	char path[600];

	snprintf(path, sizeof path, "%s%s", root, rel);
	if (symlink(target, path) != 0)
		perror("symlink");
}

/* partición NAME del disco DISK: inicio y tamaño en MiB, número de dispositivo y propiedades de udev */
static void part(const char *dev, const char *disk, const char *name, long start_mb, long size_mb,
		 const char *devnum, const char *udev)
{
	char rel[300], v[40];

	snprintf(rel, sizeof rel, "/sys/devices/%s/%s/%s/partition", dev, disk, name);
	put(rel, "1\n");
	snprintf(rel, sizeof rel, "/sys/devices/%s/%s/%s/start", dev, disk, name);
	snprintf(v, sizeof v, "%ld\n", start_mb * 2048);
	put(rel, v);
	snprintf(rel, sizeof rel, "/sys/devices/%s/%s/%s/size", dev, disk, name);
	snprintf(v, sizeof v, "%ld\n", size_mb * 2048);
	put(rel, v);
	snprintf(rel, sizeof rel, "/sys/devices/%s/%s/%s/dev", dev, disk, name);
	put(rel, devnum);
	snprintf(rel, sizeof rel, "/run/udev/data/b%s", devnum);
	put(rel, udev);
}

static const struct rros_disk *find(const struct rros_disk *v, int n, const char *name)
{
	for (int i = 0; i < n; i++)
		if (!strcmp(v[i].name, name))
			return &v[i];
	return NULL;
}

static struct rros_disk v[8];

int main(void)
{
	const struct rros_disk *d;
	char sys[64];
	int n;

	snprintf(root, sizeof root, "/tmp/rros-diskstest-XXXXXX");
	if (!mkdtemp(root))
		return 1;

	/* NVMe de 100 GiB con Windows: ESP 100 MiB, MSR 16 MiB, NTFS 60 GiB, recuperación 600 MiB al final
	 * y un hueco de ~39 GiB entre el NTFS y la recuperación (el que deja «Reducir volumen») */
	put("/sys/devices/pci/nvme0n1/size", "209715200\n");
	put("/sys/devices/pci/nvme0n1/queue/rotational", "0\n");
	put("/sys/devices/pci/nvme0n1/device/model", "WD Blue SN570\n");
	put("/sys/devices/pci/nvme0n1/dev", "259:0\n");
	put("/run/udev/data/b259:0", "E:ID_PART_TABLE_TYPE=gpt\n");
	part("pci", "nvme0n1", "nvme0n1p1", 1, 100, "259:1",
	     "E:ID_FS_TYPE=vfat\nE:ID_PART_ENTRY_TYPE=C12A7328-F81F-11D2-BA4B-00A0C93EC93B\n");
	part("pci", "nvme0n1", "nvme0n1p2", 101, 16, "259:2",
	     "E:ID_PART_ENTRY_TYPE=e3c9e316-0b5c-4db8-817d-f92df00215ae\n");
	part("pci", "nvme0n1", "nvme0n1p3", 117, 60 * 1024, "259:3",
	     "E:ID_FS_TYPE=ntfs\nE:ID_FS_LABEL=Windows\nE:ID_PART_ENTRY_TYPE=ebd0a0a2-b9e5-4433-87c0-68b6b72699c7\n");
	part("pci", "nvme0n1", "nvme0n1p4", 102400 - 1 - 600, 600, "259:4",
	     "E:ID_FS_TYPE=ntfs\nE:ID_PART_ENTRY_TYPE=de94bba4-06d1-4d40-a16a-bfd50179d6ac\n");
	put("/sys/block/.keep", "");
	link_to("/sys/block/nvme0n1", "../devices/pci/nvme0n1");

	/* disco duro vacío (sin tabla) de 500 GiB */
	put("/sys/devices/ata/sda/size", "1048576000\n");
	put("/sys/devices/ata/sda/queue/rotational", "1\n");
	put("/sys/devices/ata/sda/device/vendor", "ATA     \n");
	put("/sys/devices/ata/sda/device/model", "ST500DM002\n");
	put("/sys/devices/ata/sda/dev", "8:0\n");
	link_to("/sys/block/sda", "../devices/ata/sda");

	/* memoria USB con el sistema live (montada en /cdrom) */
	put("/sys/devices/usb1/sdb/size", "62914560\n");
	put("/sys/devices/usb1/sdb/dev", "8:16\n");
	put("/run/udev/data/b8:16", "E:ID_PART_TABLE_TYPE=dos\n");
	part("usb1", "sdb", "sdb1", 1, 4000, "8:17", "E:ID_FS_TYPE=iso9660\nE:ID_PART_ENTRY_TYPE=0x0\n");
	link_to("/sys/block/sdb", "../devices/usb1/sdb");

	/* disco con Linux (BitLocker no) y una partición de intercambio activada */
	put("/sys/devices/ata/sdc/size", "41943040\n");
	put("/sys/devices/ata/sdc/dev", "8:32\n");
	put("/run/udev/data/b8:32", "E:ID_PART_TABLE_TYPE=gpt\n");
	part("ata", "sdc", "sdc1", 1, 18000, "8:33", "E:ID_FS_TYPE=ext4\nE:ID_PART_ENTRY_TYPE=0fc63daf-8483-4772-8e79-3d69d8477de4\n");
	part("ata", "sdc", "sdc2", 18001, 2000, "8:34", "E:ID_FS_TYPE=swap\n");
	link_to("/sys/block/sdc", "../devices/ata/sdc");

	/* cosas que no son discos para instalar */
	put("/sys/devices/virtual/loop0/size", "100\n");
	link_to("/sys/block/loop0", "../devices/virtual/loop0");
	put("/sys/devices/ata/sr0/size", "2097152\n");
	link_to("/sys/block/sr0", "../devices/ata/sr0");

	put("/proc/self/mounts", "/dev/sdb1 /cdrom iso9660 ro 0 0\n/dev/loop0 /rofs squashfs ro 0 0\n");
	put("/proc/swaps", "Filename Type Size Used Priority\n/dev/sdc2 partition 2047996 0 -2\n");

	n = rros_disks_scan(v, 8, root);
	CHECK(n == 4);
	CHECK_STR(v[0].name, "nvme0n1");   /* por nombre */

	d = find(v, n, "nvme0n1");
	CHECK(d != NULL);
	if (d) {
		CHECK_STR(d->model, "WD Blue SN570");
		CHECK_STR(d->kind, "NVMe");
		CHECK_STR(d->pttype, "gpt");
		CHECK(d->nparts == 4);
		CHECK(d->windows && !d->linux_os && !d->bitlocker);
		CHECK(d->esp >= 0 && !strcmp(d->parts[d->esp].name, "nvme0n1p1"));
		CHECK(d->parts[d->esp].start == RROS_MIB && d->parts[d->esp].size == 100 * RROS_MIB);
		CHECK_STR(d->in_use, "");
		/* el hueco entre el NTFS y la recuperación */
		CHECK(d->free_start == (117ULL + 60 * 1024) * RROS_MIB);
		CHECK(d->free_size == (102400ULL - 1 - 600 - 117 - 60 * 1024) * RROS_MIB);
		rros_disk_systems(d, sys, sizeof sys);
		CHECK_STR(sys, "Windows");
	}

	d = find(v, n, "sda");
	CHECK(d != NULL);
	if (d) {
		CHECK_STR(d->model, "ATA ST500DM002");
		CHECK_STR(d->kind, "disco duro");
		CHECK_STR(d->pttype, "");
		CHECK(d->nparts == 0 && d->esp == -1);
		CHECK(d->free_start == RROS_MIB && d->free_size == 500ULL * 1024 * RROS_MIB - 2 * RROS_MIB);
	}

	d = find(v, n, "sdb");
	CHECK(d != NULL);
	if (d) {
		CHECK_STR(d->kind, "USB");
		CHECK(strstr(d->in_use, "/cdrom") != NULL);
	}

	d = find(v, n, "sdc");
	CHECK(d != NULL);
	if (d) {
		CHECK(d->linux_os && !d->windows);
		CHECK(strstr(d->in_use, "intercambio") != NULL);
		CHECK(d->free_size == 478 * RROS_MIB);   /* 18000 + 2000 MiB de 20480, menos el MiB del final */
		rros_disk_systems(d, sys, sizeof sys);
		CHECK_STR(sys, "Linux");
	}

	/* hueco calculado a mano */
	{
		struct rros_disk x = { .size = 10 * 1024 * RROS_MIB, .nparts = 2 };

		x.parts[0] = (struct rros_dpart){ .start = 5000 * RROS_MIB + 7, .size = 1000 * RROS_MIB };
		x.parts[1] = (struct rros_dpart){ .start = RROS_MIB, .size = 1000 * RROS_MIB };
		rros_disk_free(&x);
		/* entre 1001 MiB y 5000 MiB (3999) contra después de 6001 MiB (4238): gana el final, alineado */
		CHECK(x.free_start == 6001 * RROS_MIB);
		CHECK(x.free_size == (10239ULL - 6001) * RROS_MIB);
		x.size = 0;
		x.nparts = 0;
		rros_disk_free(&x);
		CHECK(x.free_size == 0);
	}

	{
		char cmd[300];

		snprintf(cmd, sizeof cmd, "rm -rf '%s'", root);
		if (system(cmd) != 0)
			return 1;
	}
	DONE();
}
