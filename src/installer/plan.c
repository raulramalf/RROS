#include "plan.h"

#include <stdio.h>
#include <string.h>

#define BIOS_BOOT_GUID "21686148-6449-6E6F-744E-656564454649"

int rros_plan_make(struct rros_plan *p, const char *profile, long disk_mb,
		   int uefi, long min_disk_mb, char *err, size_t en)
{
	/* ext4 en Lite (menos RAM y CPU); btrfs con snapshots en el resto */
	const char *rootfs = strcmp(profile, "lite") == 0 ? "ext4" : "btrfs";
	int n = 0;

	if (disk_mb < min_disk_mb) {
		snprintf(err, en, "disco de %ld MB; el perfil '%s' necesita al menos %ld MB",
			 disk_mb, profile, min_disk_mb);
		return -1;
	}
	memset(p, 0, sizeof *p);
	if (uefi)
		p->parts[n++] = (struct rros_part){ "ESP", "vfat", 512, "/boot/efi", "U" };
	else
		p->parts[n++] = (struct rros_part){ "bios_boot", "-", 1, "-", BIOS_BOOT_GUID };
	p->parts[n++] = (struct rros_part){ "rros-root", rootfs, 0, "/", "L" };
	p->n = n;
	return 0;
}

void rros_plan_sfdisk(const struct rros_plan *p, char *out, size_t n)
{
	size_t used = (size_t)snprintf(out, n, "label: gpt\n");

	for (int i = 0; i < p->n && used < n; i++) {
		const struct rros_part *x = &p->parts[i];
		int w;

		if (x->size_mb)
			w = snprintf(out + used, n - used, "size=%ldMiB, type=%s, name=%s\n",
				     x->size_mb, x->sfdisk_type, x->label);
		else
			w = snprintf(out + used, n - used, "type=%s, name=%s\n",
				     x->sfdisk_type, x->label);
		if (w < 0)
			break;
		used += (size_t)w;
	}
}
