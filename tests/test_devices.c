/* Inventario de dispositivos sobre un /sys, /proc y /run falsos. */
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "check.h"
#include "rros_devices.h"

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

static void dir(const char *rel)
{
	put(rel, "");   /* crea los padres */
	{
		char path[600];

		snprintf(path, sizeof path, "%s%s", root, rel);
		unlink(path);
		mkdir(path, 0755);
	}
}

static void link_to(const char *rel, const char *target)
{
	char path[600];

	snprintf(path, sizeof path, "%s%s", root, rel);
	if (symlink(target, path) != 0)
		perror("symlink");
}

static const struct rros_dev *get(const struct rros_devlist *l, const char *id)
{
	return rros_devs_find(l, id);
}

int main(void)
{
	struct rros_devlist l;
	const struct rros_dev *d;
	char sz[32];

	snprintf(root, sizeof root, "/tmp/rros-devtest-XXXXXX");
	if (!mkdtemp(root))
		return 1;

	/* disco interno SSD con dos particiones (una es /) */
	put("/sys/devices/pci0000:00/nvme/nvme0n1/size", "1000215216\n");
	put("/sys/devices/pci0000:00/nvme/nvme0n1/removable", "0\n");
	put("/sys/devices/pci0000:00/nvme/nvme0n1/queue/rotational", "0\n");
	put("/sys/devices/pci0000:00/nvme/nvme0n1/device/model", "Samsung SSD 980   \n");
	put("/sys/devices/pci0000:00/nvme/nvme0n1/dev", "259:0\n");
	put("/sys/devices/pci0000:00/nvme/nvme0n1/nvme0n1p1/partition", "1\n");
	put("/sys/devices/pci0000:00/nvme/nvme0n1/nvme0n1p1/size", "1048576\n");
	put("/sys/devices/pci0000:00/nvme/nvme0n1/nvme0n1p1/dev", "259:1\n");
	put("/sys/devices/pci0000:00/nvme/nvme0n1/nvme0n1p2/partition", "2\n");
	put("/sys/devices/pci0000:00/nvme/nvme0n1/nvme0n1p2/size", "999000000\n");
	put("/sys/devices/pci0000:00/nvme/nvme0n1/nvme0n1p2/dev", "259:2\n");
	dir("/sys/block");
	link_to("/sys/block/nvme0n1", "../devices/pci0000:00/nvme/nvme0n1");
	/* memoria USB con una partición FAT montada por udisks y otra sin montar */
	put("/sys/devices/pci0000:00/usb1/1-2/host6/sdb/size", "62521344\n");
	put("/sys/devices/pci0000:00/usb1/1-2/host6/sdb/removable", "1\n");
	put("/sys/devices/pci0000:00/usb1/1-2/host6/sdb/device/vendor", "Kingston\n");
	put("/sys/devices/pci0000:00/usb1/1-2/host6/sdb/device/model", "DataTraveler 3.0\n");
	put("/sys/devices/pci0000:00/usb1/1-2/host6/sdb/sdb1/partition", "1\n");
	put("/sys/devices/pci0000:00/usb1/1-2/host6/sdb/sdb1/size", "40000000\n");
	put("/sys/devices/pci0000:00/usb1/1-2/host6/sdb/sdb1/dev", "8:17\n");
	put("/sys/devices/pci0000:00/usb1/1-2/host6/sdb/sdb2/partition", "2\n");
	put("/sys/devices/pci0000:00/usb1/1-2/host6/sdb/sdb2/size", "20000000\n");
	put("/sys/devices/pci0000:00/usb1/1-2/host6/sdb/sdb2/dev", "8:18\n");
	link_to("/sys/block/sdb", "../devices/pci0000:00/usb1/1-2/host6/sdb");
	/* lector de DVD vacío, un loop y zram que no deben aparecer */
	put("/sys/devices/ata/sr0/size", "0\n");
	put("/sys/devices/ata/sr0/removable", "1\n");
	link_to("/sys/block/sr0", "../devices/ata/sr0");
	put("/sys/devices/virtual/loop0/size", "100\n");
	link_to("/sys/block/loop0", "../devices/virtual/loop0");
	put("/sys/devices/virtual/zram0/size", "100\n");
	link_to("/sys/block/zram0", "../devices/virtual/zram0");
	put("/run/udev/data/b8:17", "E:ID_FS_TYPE=vfat\nE:ID_FS_LABEL=FOTOS\n");
	put("/run/udev/data/b8:18", "E:ID_FS_TYPE=ext4\n");
	put("/run/udev/data/b259:2", "E:ID_FS_TYPE=ext4\n");
	put("/proc/self/mounts", "/dev/nvme0n1p2 / ext4 rw 0 0\n/dev/sdb1 /media/ana/FOTOS\\040USB vfat rw 0 0\n");

	/* USB: un concentrador (no sale) y un ratón */
	put("/sys/bus/usb/devices/usb1/idVendor", "1d6b\n");
	put("/sys/bus/usb/devices/usb1/bDeviceClass", "09\n");
	put("/sys/bus/usb/devices/1-1/idVendor", "046d\n");
	put("/sys/bus/usb/devices/1-1/idProduct", "c52b\n");
	put("/sys/bus/usb/devices/1-1/bDeviceClass", "00\n");
	put("/sys/bus/usb/devices/1-1/manufacturer", "Logitech\n");
	put("/sys/bus/usb/devices/1-1/product", "USB Receiver\n");
	put("/sys/bus/usb/devices/1-1/speed", "12\n");
	put("/sys/bus/usb/devices/1-1:1.0/bInterfaceClass", "03\n");

	/* red: Wi-Fi conectada, Ethernet sin cable, un puente virtual (no sale) */
	put("/sys/class/net/wlp2s0/device/vendor", "0x8086\n");
	put("/sys/class/net/wlp2s0/wireless/x", "");
	put("/sys/class/net/wlp2s0/operstate", "up\n");
	put("/sys/class/net/wlp2s0/address", "aa:bb:cc:dd:ee:ff\n");
	put("/sys/class/net/enp3s0/device/vendor", "0x10ec\n");
	put("/sys/class/net/enp3s0/operstate", "down\n");
	put("/sys/class/net/docker0/operstate", "up\n");
	put("/sys/class/net/lo/operstate", "unknown\n");

	/* radios */
	put("/sys/class/rfkill/rfkill0/type", "wlan\n");
	put("/sys/class/rfkill/rfkill0/name", "phy0\n");
	put("/sys/class/rfkill/rfkill0/soft", "0\n");
	put("/sys/class/rfkill/rfkill0/hard", "0\n");
	put("/sys/class/rfkill/rfkill1/type", "bluetooth\n");
	put("/sys/class/rfkill/rfkill1/name", "hci0\n");
	put("/sys/class/rfkill/rfkill1/soft", "1\n");
	put("/sys/class/rfkill/rfkill1/hard", "0\n");

	/* sonido, pantallas, batería, entrada, cámara, PCI */
	put("/proc/asound/cards", " 0 [PCH            ]: HDA-Intel - HDA Intel PCH\n"
				  "                      HDA Intel PCH at 0xf1230000 irq 147\n"
				  " 1 [HDMI           ]: HDA-Intel - HDA Intel HDMI\n"
				  "                      HDA Intel HDMI at 0xf1234000 irq 148\n");
	put("/sys/class/drm/card0-eDP-1/status", "connected\n");
	put("/sys/class/drm/card0-eDP-1/modes", "1920x1080\n1280x720\n");
	put("/sys/class/drm/card0-HDMI-A-1/status", "disconnected\n");
	put("/sys/class/drm/card0/dev", "226:0\n");
	put("/sys/class/power_supply/BAT0/type", "Battery\n");
	put("/sys/class/power_supply/BAT0/capacity", "87\n");
	put("/sys/class/power_supply/BAT0/status", "Discharging\n");
	put("/sys/class/power_supply/BAT0/energy_full", "45000000\n");
	put("/sys/class/power_supply/BAT0/energy_full_design", "50000000\n");
	put("/sys/class/power_supply/AC/type", "Mains\n");
	put("/sys/class/power_supply/AC/online", "0\n");
	put("/proc/bus/input/devices",
	    "I: Bus=0011\nN: Name=\"AT Translated Set 2 keyboard\"\nH: Handlers=sysrq kbd event0\n\n"
	    "I: Bus=0018\nN: Name=\"SYNA2B46:00 06CB:CD5F Touchpad\"\nH: Handlers=mouse1 event7\n\n"
	    "I: Bus=0019\nN: Name=\"Power Button\"\nH: Handlers=kbd event2\n\n"
	    "I: Bus=0003\nN: Name=\"Logitech USB Receiver\"\nH: Handlers=mouse0 event3\n");
	put("/sys/class/video4linux/video0/name", "Integrated Camera\n");
	put("/sys/class/video4linux/video0/index", "0\n");
	put("/sys/class/video4linux/video1/name", "Integrated Camera\n");
	put("/sys/class/video4linux/video1/index", "1\n");
	put("/sys/bus/pci/devices/0000:00:02.0/class", "0x030000\n");
	put("/sys/bus/pci/devices/0000:00:02.0/vendor", "0x8086\n");
	put("/sys/bus/pci/devices/0000:00:02.0/device", "0x9a49\n");
	put("/sys/bus/pci/devices/0000:00:02.0/driver/x", "");
	put("/sys/bus/pci/devices/0000:00:00.0/class", "0x060000\n");   /* puente: no sale */
	put("/usr/share/misc/pci.ids", "# comentario\n8086  Intel Corporation\n\t9a49  TigerLake-LP GT2 [Iris Xe Graphics]\n"
				       "10ec  Realtek Semiconductor Co., Ltd.\n");

	rros_devs_scan(&l, root);

	/* discos */
	d = get(&l, "nvme0n1");
	CHECK(d && !strcmp(d->name, "Samsung SSD 980") && strstr(d->detail, "NVMe") && !*d->actions);
	d = get(&l, "nvme0n1p2");
	CHECK(d && !strcmp(d->parent, "nvme0n1") && !strncmp(d->state, "sistema:", 8) && !*d->actions);
	d = get(&l, "sdb");
	CHECK(d && !strcmp(d->name, "Kingston DataTraveler 3.0") && strstr(d->detail, "USB") && !strcmp(d->actions, "eject"));
	d = get(&l, "sdb1");
	CHECK(d && !strcmp(d->name, "FOTOS") && strstr(d->detail, "vfat") && !strcmp(d->actions, "unmount"));
	CHECK(d && strstr(d->state, "/media/ana/FOTOS USB") != NULL);   /* \040 decodificado */
	d = get(&l, "sdb2");
	CHECK(d && !strcmp(d->actions, "mount") && !strcmp(d->state, "sin montar"));
	d = get(&l, "sr0");
	CHECK(d && strstr(d->detail, "lector óptico") && !strcmp(d->state, "vacío") && !strcmp(d->actions, "eject"));
	CHECK(get(&l, "loop0") == NULL && get(&l, "zram0") == NULL);
	/* el disco va justo antes de sus particiones */
	{
		size_t i;

		for (i = 0; i < l.n && strcmp(l.v[i].id, "sdb"); i++)
			;
		CHECK(i + 2 < l.n && !strcmp(l.v[i + 1].id, "sdb1") && !strcmp(l.v[i + 2].id, "sdb2"));
	}

	/* USB */
	d = get(&l, "1-1");
	CHECK(d && !strcmp(d->name, "Logitech USB Receiver") && strstr(d->detail, "046d:c52b"));
	CHECK(get(&l, "usb1") == NULL && get(&l, "1-1:1.0") == NULL);

	/* red */
	d = get(&l, "wlp2s0");
	CHECK(d && !strcmp(d->name, "Wi-Fi (wlp2s0)") && !strcmp(d->state, "conectado"));
	d = get(&l, "enp3s0");
	CHECK(d && !strcmp(d->state, "sin cable"));
	CHECK(get(&l, "docker0") == NULL && get(&l, "lo") == NULL);

	/* radios */
	d = get(&l, "rfkill0");
	CHECK(d && !strcmp(d->name, "Wi-Fi") && !strcmp(d->state, "activo") && !strcmp(d->actions, "block"));
	d = get(&l, "rfkill1");
	CHECK(d && !strcmp(d->name, "Bluetooth") && !strcmp(d->state, "desactivado") && !strcmp(d->actions, "unblock"));

	/* resto */
	d = get(&l, "card0");
	CHECK(d && !strcmp(d->name, "HDA Intel PCH"));
	CHECK(get(&l, "card1") != NULL);
	d = get(&l, "card0-eDP-1");
	CHECK(d && !strcmp(d->name, "eDP-1") && strstr(d->detail, "1920x1080"));
	CHECK(get(&l, "card0-HDMI-A-1") == NULL);
	d = get(&l, "BAT0");
	CHECK(d && strstr(d->detail, "87%") && strstr(d->detail, "salud 90%") && !strcmp(d->state, "descargando"));
	d = get(&l, "AC");
	CHECK(d && !strcmp(d->state, "sin enchufar"));
	{
		int kbd = 0, touch = 0, mouse = 0, power = 0;

		for (size_t i = 0; i < l.n; i++) {
			if (strcmp(l.v[i].cat, "entrada"))
				continue;
			kbd += !strcmp(l.v[i].name, "Teclado");
			touch += !strcmp(l.v[i].name, "Touchpad");
			mouse += !strcmp(l.v[i].name, "Ratón");
			power += strstr(l.v[i].detail, "Power Button") != NULL;
		}
		CHECK(kbd == 1 && touch == 1 && mouse == 1 && power == 0);
	}
	d = get(&l, "video0");
	CHECK(d && !strcmp(d->name, "Integrated Camera"));
	CHECK(get(&l, "video1") == NULL);
	d = get(&l, "0000:00:02.0");
	CHECK(d && !strcmp(d->name, "Gráfica") && !strcmp(d->detail, "Intel Corporation · TigerLake-LP GT2 [Iris Xe Graphics]")
	      && !strcmp(d->state, "con controlador"));
	CHECK(get(&l, "0000:00:00.0") == NULL);

	/* utilidades */
	rros_human_size(62521344ULL * 512, sz, sizeof sz);
	CHECK(!strcmp(sz, "32.0 GB"));
	rros_human_size(512ULL * 1000 * 1000, sz, sizeof sz);
	CHECK(!strcmp(sz, "512 MB"));
	CHECK(rros_mount_is_system("/") && rros_mount_is_system("/boot/efi") && rros_mount_is_system("/mnt/datos"));
	CHECK(!rros_mount_is_system("/media/ana/USB") && !rros_mount_is_system("/run/media/ana/X"));

	/* un /sys vacío no rompe nada */
	{
		struct rros_devlist e;

		rros_devs_scan(&e, "/nonexistent-rros");
		CHECK(e.n == 0);
		rros_devs_free(&e);
	}
	rros_devs_free(&l);
	{
		char cmd[300];

		snprintf(cmd, sizeof cmd, "rm -rf '%s'", root);
		if (system(cmd) != 0)
			fprintf(stderr, "no se pudo borrar %s\n", root);
	}
	DONE();
}
