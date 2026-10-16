/* Detección de hardware. `root` permite apuntar a un árbol falso (tests). */
#ifndef RROS_HW_H
#define RROS_HW_H

enum rros_gpu {
	RROS_GPU_NONE,
	RROS_GPU_INTEL,
	RROS_GPU_AMD,
	RROS_GPU_NVIDIA,
	RROS_GPU_OTHER
};

struct rros_hw {
	long ram_mb;
	int cpus;
	char arch[80];
	char cpu_model[128];
	enum rros_gpu gpu;
	int uefi;
};

/* 0 ok, -1 si no se puede leer la memoria */
int rros_hw_detect(struct rros_hw *hw, const char *root);
const char *rros_gpu_name(enum rros_gpu g);
/* "lite", "standard" o "pro" */
const char *rros_hw_pick_profile(const struct rros_hw *hw);
/* MemTotal - MemAvailable en MB, o -1 */
long rros_hw_ram_used_mb(const char *root);

#endif
