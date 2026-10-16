/* Inventario de dispositivos leyendo /sys, /proc y la base de datos de udev (sin programas externos).
 * `root` antepone un directorio a todas las rutas (las pruebas montan un /sys falso). */
#ifndef RROS_DEVICES_H
#define RROS_DEVICES_H

#include <stddef.h>

struct rros_dev {
	char cat[16];        /* disco, usb, red, radio, sonido, pantalla, energia, entrada, camara, tarjeta */
	char id[48];         /* identificador para las acciones: sdb1, rfkill0, ens3... */
	char parent[48];     /* para particiones: el disco */
	char name[128];      /* nombre legible */
	char detail[256];    /* tamaño, velocidad, modelo... */
	char state[64];      /* montado en /media/..., conectado, bloqueado... */
	char actions[48];    /* acciones posibles separadas por comas: mount,unmount,eject,block,unblock */
};

struct rros_devlist {
	struct rros_dev *v;
	size_t n, cap;
};

void rros_devs_scan(struct rros_devlist *l, const char *root);
void rros_devs_free(struct rros_devlist *l);
const struct rros_dev *rros_devs_find(const struct rros_devlist *l, const char *id);

/* Categorías en el orden en que se muestran, con su nombre. */
extern const char *const rros_devcats[][2];
extern const size_t rros_devcats_len;

/* "1.5 GB", "512 MB"... */
void rros_human_size(unsigned long long bytes, char *buf, size_t n);

/* Montaje del sistema (/, /boot, /home...): nunca se desmonta desde aquí. */
int rros_mount_is_system(const char *mountpoint);

#endif
