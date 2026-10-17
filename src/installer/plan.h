/* Plan de particionado (solo cálculo, nunca toca discos). */
#ifndef RROS_PLAN_H
#define RROS_PLAN_H

#include <stddef.h>

struct rros_part {
	const char *role;        /* esp, bios_boot, boot, root */
	const char *label;       /* nombre de la partición GPT */
	const char *fs;          /* vfat, ext4, btrfs, - */
	long start_mb, size_mb;  /* posición en el disco, en MiB */
	const char *mount;
	const char *sfdisk_type; /* alias o GUID de sfdisk */
	int reuse;               /* ya existe (la ESP de Windows): no se crea ni se formatea */
	int crypt;               /* LUKS2 debajo del sistema de archivos */
};

struct rros_plan {
	struct rros_part parts[4];
	int n;
	int append;              /* junto a otro sistema: se añaden particiones sin tocar las que hay */
};

struct rros_plan_req {
	const char *profile;
	int uefi, encrypt;
	long disk_mb;            /* borrar todo: tamaño del disco */
	long min_mb;             /* espacio que necesita el perfil */
	/* junto a otro sistema (alongside): mayor hueco libre de un disco GPT */
	int alongside;
	long free_start_mb, free_mb;
	long esp_start_mb, esp_mb;   /* ESP que ya hay en el disco (esp_mb 0 = ninguna) */
	int have_bios_boot;
};

#define RROS_ESP_MB  512
#define RROS_BOOT_MB 1024    /* /boot sin cifrar: GRUB no abre LUKS2 */

/* 0 ok; -1 no cabe (err explica por qué) */
int rros_plan_build(struct rros_plan *p, const struct rros_plan_req *r, char *err, size_t en);
/* Script para `sfdisk` (GPT); con p->append solo las particiones nuevas, para `sfdisk --append`. */
void rros_plan_sfdisk(const struct rros_plan *p, char *out, size_t n);
/* Una línea por partición para el instalador: "rol inicio_mb tamaño_mb fs montaje reutilizada cifrada" */
void rros_plan_table(const struct rros_plan *p, char *out, size_t n);

#endif
