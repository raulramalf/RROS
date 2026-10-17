#include "plan.h"

#include <stdio.h>
#include <string.h>

#define BIOS_BOOT_GUID "21686148-6449-6E6F-744E-656564454649"

static void put(struct rros_plan *p, const char *role, const char *label, const char *fs, long start,
		long size, const char *mount, const char *type)
{
	p->parts[p->n++] = (struct rros_part){ role, label, fs, start, size, mount, type, 0, 0 };
}

int rros_plan_build(struct rros_plan *p, const struct rros_plan_req *r, char *err, size_t en)
{
	/* ext4 en Lite (menos RAM y CPU); btrfs con snapshots en el resto */
	const char *rootfs = strcmp(r->profile, "lite") == 0 ? "ext4" : "btrfs";
	long cur, end, extra = 0;

	memset(p, 0, sizeof *p);
	if (r->alongside) {
		/* el hueco ya viene alineado a 1 MiB y sin el final del disco (copia de la tabla GPT) */
		cur = r->free_start_mb;
		end = r->free_start_mb + r->free_mb;
		p->append = 1;
	} else {
		cur = 1;
		end = r->disk_mb - 1;
	}
	if (r->uefi && !(r->alongside && r->esp_mb > 0))
		extra += RROS_ESP_MB;
	if (!r->uefi && !(r->alongside && r->have_bios_boot))
		extra += 1;
	if (r->encrypt)
		extra += RROS_BOOT_MB;
	if (end - cur - extra < r->min_mb) {
		long have = end - cur - extra;

		/* redondeo hacia abajo: «8.0 GB, necesita 8.0 GB» confundiría */
		snprintf(err, en, "%s de %.1f GB; el perfil '%s' necesita al menos %.1f GB%s",
			 r->alongside ? "espacio libre" : "disco", (have > 0 ? have * 10 / 1024 : 0) / 10.0,
			 r->profile, r->min_mb / 1024.0, r->encrypt ? " (más 1 GB para /boot al cifrar)" : "");
		return -1;
	}

	if (r->uefi && r->alongside && r->esp_mb > 0) {
		put(p, "esp", "ESP", "vfat", r->esp_start_mb, r->esp_mb, "/boot/efi", "U");
		p->parts[p->n - 1].reuse = 1;
	} else if (r->uefi) {
		put(p, "esp", "ESP", "vfat", cur, RROS_ESP_MB, "/boot/efi", "U");
		cur += RROS_ESP_MB;
	} else if (!(r->alongside && r->have_bios_boot)) {
		put(p, "bios_boot", "bios_boot", "-", cur, 1, "-", BIOS_BOOT_GUID);
		cur += 1;
	}
	if (r->encrypt) {
		put(p, "boot", "rros-boot", "ext4", cur, RROS_BOOT_MB, "/boot", "L");
		cur += RROS_BOOT_MB;
	}
	put(p, "root", "rros-root", rootfs, cur, end - cur, "/", "L");
	p->parts[p->n - 1].crypt = r->encrypt;
	return 0;
}

void rros_plan_sfdisk(const struct rros_plan *p, char *out, size_t n)
{
	size_t used = 0;
	int w = 0;

	if (n)
		*out = '\0';
	if (!p->append)
		w = snprintf(out, n, "label: gpt\n");
	if (w > 0)
		used = (size_t)w;
	for (int i = 0; i < p->n && used < n; i++) {
		const struct rros_part *x = &p->parts[i];

		if (x->reuse)
			continue;
		w = snprintf(out + used, n - used, "start=%ldMiB, size=%ldMiB, type=%s, name=%s\n",
			     x->start_mb, x->size_mb, x->sfdisk_type, x->label);
		if (w < 0)
			break;
		used += (size_t)w;
	}
}

void rros_plan_table(const struct rros_plan *p, char *out, size_t n)
{
	size_t used = 0;

	if (n)
		*out = '\0';
	for (int i = 0; i < p->n && used < n; i++) {
		const struct rros_part *x = &p->parts[i];
		int w = snprintf(out + used, n - used, "%s %ld %ld %s %s %d %d\n", x->role, x->start_mb,
				 x->size_mb, x->fs, x->mount, x->reuse, x->crypt);

		if (w < 0)
			break;
		used += (size_t)w;
	}
}
