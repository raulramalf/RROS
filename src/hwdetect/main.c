/* rros-hwdetect: lee la RAM y CPU del sistema y propone un perfil RROS. */
#include <stdio.h>
#include <string.h>

static long mem_total_mb(void)
{
    FILE *f = fopen("/proc/meminfo", "r");
    char line[256];
    long kb = 0;

    if (!f)
        return -1;
    while (fgets(line, sizeof line, f))
        if (sscanf(line, "MemTotal: %ld kB", &kb) == 1)
            break;
    fclose(f);
    return kb / 1024;
}

static int cpu_count(void)
{
    FILE *f = fopen("/proc/cpuinfo", "r");
    char line[256];
    int n = 0;

    if (!f)
        return -1;
    while (fgets(line, sizeof line, f))
        if (strncmp(line, "processor", 9) == 0)
            n++;
    fclose(f);
    return n;
}

static const char *pick_profile(long ram_mb)
{
    if (ram_mb >= 6144)
        return "pro";
    if (ram_mb >= 3072)
        return "standard";
    return "lite";
}

int main(void)
{
    long ram = mem_total_mb();
    int cpus = cpu_count();

    if (ram < 0 || cpus < 0) {
        fprintf(stderr, "rros-hwdetect: no se pudo leer /proc\n");
        return 1;
    }
    printf("ram_mb=%ld\ncpus=%d\nprofile=%s\n", ram, cpus, pick_profile(ram));
    return 0;
}
