#include "rros_hw.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <unistd.h>

static int read_first_line(const char *path, char *buf, size_t n)
{
	FILE *f = fopen(path, "r");
	char *r;

	if (!f)
		return -1;
	r = fgets(buf, (int)n, f);
	fclose(f);
	if (!r)
		return -1;
	buf[strcspn(buf, "\r\n")] = '\0';
	return 0;
}

static long meminfo_kb(const char *root, const char *field)
{
	char path[512], line[256], fmt[64];
	long kb = -1;
	FILE *f;

	snprintf(path, sizeof path, "%s/proc/meminfo", root);
	snprintf(fmt, sizeof fmt, "%s: %%ld kB", field);
	f = fopen(path, "r");
	if (!f)
		return -1;
	while (fgets(line, sizeof line, f))
		if (sscanf(line, fmt, &kb) == 1)
			break;
	fclose(f);
	return kb;
}

long rros_hw_ram_used_mb(const char *root)
{
	long total, avail;

	if (!root)
		root = "";
	total = meminfo_kb(root, "MemTotal");
	avail = meminfo_kb(root, "MemAvailable");
	if (total < 0 || avail < 0)
		return -1;
	return (total - avail) / 1024;
}

static void detect_cpu(struct rros_hw *hw, const char *root)
{
	char path[512], line[512];
	FILE *f;

	snprintf(path, sizeof path, "%s/proc/cpuinfo", root);
	f = fopen(path, "r");
	if (f) {
		while (fgets(line, sizeof line, f)) {
			const char *colon;

			if (strncmp(line, "processor", 9) == 0)
				hw->cpus++;
			if (hw->cpu_model[0])
				continue;
			if (strncmp(line, "model name", 10) == 0 ||
			    strncmp(line, "Model", 5) == 0 ||
			    strncmp(line, "Hardware", 8) == 0) {
				colon = strchr(line, ':');
				if (colon) {
					colon++;
					while (*colon == ' ')
						colon++;
					snprintf(hw->cpu_model, sizeof hw->cpu_model, "%s", colon);
					hw->cpu_model[strcspn(hw->cpu_model, "\r\n")] = '\0';
				}
			}
		}
		fclose(f);
	}
	if (hw->cpus < 1)
		hw->cpus = 1;
}

static int gpu_rank(enum rros_gpu g)
{
	switch (g) {
	case RROS_GPU_NVIDIA: return 4;
	case RROS_GPU_AMD:    return 3;
	case RROS_GPU_INTEL:  return 2;
	case RROS_GPU_OTHER:  return 1;
	default:              return 0;
	}
}

static void detect_gpu(struct rros_hw *hw, const char *root)
{
	char dir[512], path[1024], line[64];
	struct dirent *e;
	DIR *d;

	snprintf(dir, sizeof dir, "%s/sys/class/drm", root);
	d = opendir(dir);
	if (!d)
		return;
	while ((e = readdir(d))) {
		const char *p = e->d_name + 4;
		enum rros_gpu g;

		if (strncmp(e->d_name, "card", 4) != 0 || !*p)
			continue;
		if (strspn(p, "0123456789") != strlen(p))
			continue;
		snprintf(path, sizeof path, "%s/%s/device/vendor", dir, e->d_name);
		if (read_first_line(path, line, sizeof line) != 0)
			continue;
		if (strcmp(line, "0x8086") == 0)
			g = RROS_GPU_INTEL;
		else if (strcmp(line, "0x1002") == 0)
			g = RROS_GPU_AMD;
		else if (strcmp(line, "0x10de") == 0)
			g = RROS_GPU_NVIDIA;
		else
			g = RROS_GPU_OTHER;
		if (gpu_rank(g) > gpu_rank(hw->gpu))
			hw->gpu = g;
	}
	closedir(d);
}

int rros_hw_detect(struct rros_hw *hw, const char *root)
{
	struct utsname u;
	char path[512];
	struct stat st;

	if (!root)
		root = "";
	memset(hw, 0, sizeof *hw);
	hw->ram_mb = meminfo_kb(root, "MemTotal");
	if (hw->ram_mb < 0)
		return -1;
	hw->ram_mb /= 1024;
	detect_cpu(hw, root);
	detect_gpu(hw, root);
	snprintf(path, sizeof path, "%s/sys/firmware/efi", root);
	hw->uefi = stat(path, &st) == 0 && S_ISDIR(st.st_mode);
	if (uname(&u) == 0)
		snprintf(hw->arch, sizeof hw->arch, "%s", u.machine);
	else
		snprintf(hw->arch, sizeof hw->arch, "unknown");
	return 0;
}

const char *rros_gpu_name(enum rros_gpu g)
{
	switch (g) {
	case RROS_GPU_INTEL:  return "intel";
	case RROS_GPU_AMD:    return "amd";
	case RROS_GPU_NVIDIA: return "nvidia";
	case RROS_GPU_OTHER:  return "other";
	default:              return "none";
	}
}

/*
 * Umbrales sobre MemTotal (el kernel reserva parte: un equipo de 8 GB
 * reporta ~7.6 GB). CPU y GPU solo pueden bajar el perfil, nunca subirlo.
 */
const char *rros_hw_pick_profile(const struct rros_hw *hw)
{
	int level;

	if (hw->ram_mb >= 6144)
		level = 2;
	else if (hw->ram_mb >= 3072)
		level = 1;
	else
		level = 0;
	if (hw->cpus < 2)
		level = 0;
	else if (hw->cpus < 4 && level > 1)
		level = 1;
	if (hw->gpu == RROS_GPU_NONE && level > 1)
		level = 1;
	return level == 2 ? "pro" : level == 1 ? "standard" : "lite";
}
