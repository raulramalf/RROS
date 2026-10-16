#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "check.h"
#include "rros_hw.h"

static char root[128];

static void write_file(const char *rel, const char *content)
{
	char path[512], *s;
	FILE *f;

	snprintf(path, sizeof path, "%s/%s", root, rel);
	for (s = path + strlen(root) + 1; *s; s++) {
		if (*s == '/') {
			*s = '\0';
			mkdir(path, 0755);
			*s = '/';
		}
	}
	f = fopen(path, "w");
	if (f) {
		fputs(content, f);
		fclose(f);
	}
}

static void fake(long ram_kb, int cpus, const char *vendor, int uefi)
{
	char buf[4096] = "";
	char sys[512];

	snprintf(sys, sizeof sys, "rm -rf %s && mkdir -p %s", root, root);
	if (system(sys)) exit(3);
	snprintf(buf, sizeof buf, "MemTotal:       %ld kB\nMemFree: 100 kB\nMemAvailable:   %ld kB\n",
		 ram_kb, ram_kb / 2);
	write_file("proc/meminfo", buf);
	buf[0] = '\0';
	for (int i = 0; i < cpus; i++)
		strcat(buf, "processor\t: 0\nmodel name\t: Test CPU 3000\n\n");
	write_file("proc/cpuinfo", buf);
	if (vendor)
		write_file("sys/class/drm/card0/device/vendor", vendor);
	if (uefi)
		write_file("sys/firmware/efi/.keep", "");
}

static const char *pick(long ram_kb, int cpus, const char *vendor)
{
	static struct rros_hw hw;

	fake(ram_kb, cpus, vendor, 0);
	CHECK(rros_hw_detect(&hw, root) == 0);
	return rros_hw_pick_profile(&hw);
}

int main(void)
{
	struct rros_hw hw;

	snprintf(root, sizeof root, "/tmp/rros-hw-%d", (int)getpid());

	fake(7800000, 8, "0x10de\n", 1);
	CHECK(rros_hw_detect(&hw, root) == 0);
	CHECK(hw.ram_mb == 7617);
	CHECK(hw.cpus == 8);
	CHECK(hw.gpu == RROS_GPU_NVIDIA);
	CHECK(hw.uefi == 1);
	CHECK_STR(hw.cpu_model, "Test CPU 3000");
	CHECK(rros_hw_ram_used_mb(root) == 3808);

	/* RAM pura */
	CHECK_STR(pick(7800000, 8, "0x1002"), "pro");
	CHECK_STR(pick(3900000, 8, "0x8086"), "standard");
	CHECK_STR(pick(1900000, 8, "0x8086"), "lite");
	/* CPU y GPU solo bajan */
	CHECK_STR(pick(7800000, 1, "0x10de"), "lite");
	CHECK_STR(pick(7800000, 2, "0x10de"), "standard");
	CHECK_STR(pick(7800000, 8, NULL), "standard");
	CHECK_STR(pick(1900000, 8, "0x10de"), "lite");
	/* umbrales exactos */
	CHECK_STR(pick(6144 * 1024L, 4, "0x10de"), "pro");
	CHECK_STR(pick(6144 * 1024L - 1024, 4, "0x10de"), "standard");
	CHECK_STR(pick(3072 * 1024L, 4, "0x10de"), "standard");
	CHECK_STR(pick(3072 * 1024L - 1024, 4, "0x10de"), "lite");

	/* GPU: prioridad y vendor desconocido */
	fake(4000000, 4, "0x8086\n", 0);
	write_file("sys/class/drm/card1/device/vendor", "0x10de\n");
	write_file("sys/class/drm/card1-HDMI-A-1/device/vendor", "0x1002\n");
	CHECK(rros_hw_detect(&hw, root) == 0);
	CHECK(hw.gpu == RROS_GPU_NVIDIA);
	CHECK(hw.uefi == 0);
	fake(4000000, 4, "0x1234\n", 0);
	CHECK(rros_hw_detect(&hw, root) == 0);
	CHECK(hw.gpu == RROS_GPU_OTHER);

	/* sin meminfo */
	{
		char cmd[256];

		snprintf(cmd, sizeof cmd, "rm -rf %s", root);
		if (system(cmd)) return 3;
	}
	CHECK(rros_hw_detect(&hw, root) == -1);
	CHECK(rros_hw_ram_used_mb(root) == -1);
	DONE();
}
