/* Plan de particionado (solo cálculo, nunca toca discos). */
#ifndef RROS_PLAN_H
#define RROS_PLAN_H

#include <stddef.h>

struct rros_part {
	const char *label;
	const char *fs;          /* vfat, ext4, btrfs, - */
	long size_mb;            /* 0 = resto del disco */
	const char *mount;
	const char *sfdisk_type; /* alias o GUID de sfdisk */
};

struct rros_plan {
	struct rros_part parts[3];
	int n;
};

/* 0 ok; -1 disco demasiado pequeño (err) */
int rros_plan_make(struct rros_plan *p, const char *profile, long disk_mb,
		   int uefi, long min_disk_mb, char *err, size_t en);
/* Script para `sfdisk` (GPT). */
void rros_plan_sfdisk(const struct rros_plan *p, char *out, size_t n);

#endif
