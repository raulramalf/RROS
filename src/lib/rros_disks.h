/* Discos vistos por el instalador: particiones, hueco libre y otros sistemas (Windows, Linux),
 * leyendo /sys y la base de datos de udev. `root` antepone un directorio (pruebas con un /sys falso). */
#ifndef RROS_DISKS_H
#define RROS_DISKS_H

#include <stddef.h>

#define RROS_MIB (1024ULL * 1024ULL)

struct rros_dpart {
	char name[40];                     /* vda1, nvme0n1p2 */
	unsigned long long start, size;    /* bytes */
	char fstype[24], label[64];
	char ptype[40];                    /* tipo de partición: GUID (GPT) o 0x83 (MBR), en minúsculas */
	char mount[96];                    /* dónde está montada, o "" */
	int esp, bios_boot, windows, linux_os;
};

struct rros_disk {
	char name[40];                     /* vda */
	char model[128], kind[24];
	unsigned long long size;           /* bytes */
	char pttype[8];                    /* gpt, dos o "" (sin tabla) */
	char in_use[128];                  /* por qué no se puede usar ("" = libre) */
	int nparts;
	struct rros_dpart parts[32];
	unsigned long long free_start, free_size;   /* mayor hueco libre, alineado a 1 MiB */
	int esp;                           /* índice de la ESP en parts, o -1 */
	int bios_boot;                     /* hay partición BIOS boot */
	int windows, bitlocker, linux_os;     /* sistemas encontrados */
};

/* Rellena v con los discos internos y externos (sin lectores ópticos ni discos virtuales en RAM). */
int rros_disks_scan(struct rros_disk *v, int max, const char *root);
/* Recalcula el mayor hueco libre a partir de size y parts (lo usa scan; público para las pruebas). */
void rros_disk_free(struct rros_disk *d);
/* "Windows, Linux", "Windows (BitLocker)", "" */
void rros_disk_systems(const struct rros_disk *d, char *out, size_t n);

#endif
