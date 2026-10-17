#include <string.h>

#include "check.h"
#include "plan.h"

int main(void)
{
	struct rros_plan p;
	struct rros_plan_req r;
	char err[200], out[512];

	/* borrar todo, UEFI */
	r = (struct rros_plan_req){ .profile = "pro", .uefi = 1, .disk_mb = 40 * 1024, .min_mb = 22 * 1024 };
	CHECK(rros_plan_build(&p, &r, err, sizeof err) == 0);
	CHECK(p.n == 2 && !p.append);
	CHECK_STR(p.parts[0].role, "esp");
	CHECK(p.parts[0].start_mb == 1 && p.parts[0].size_mb == 512);
	CHECK_STR(p.parts[1].fs, "btrfs");
	CHECK(p.parts[1].start_mb == 513 && p.parts[1].size_mb == 40 * 1024 - 1 - 513);
	CHECK(!p.parts[1].crypt);
	rros_plan_sfdisk(&p, out, sizeof out);
	CHECK(strstr(out, "label: gpt\n") == out);
	CHECK(strstr(out, "start=1MiB, size=512MiB, type=U, name=ESP") != NULL);
	CHECK(strstr(out, "start=513MiB, size=40446MiB, type=L, name=rros-root") != NULL);
	rros_plan_table(&p, out, sizeof out);
	CHECK(strstr(out, "esp 1 512 vfat /boot/efi 0 0\n") == out);
	CHECK(strstr(out, "root 513 40446 btrfs / 0 0\n") != NULL);

	/* BIOS: partición BIOS boot de 1 MiB */
	r = (struct rros_plan_req){ .profile = "lite", .uefi = 0, .disk_mb = 8 * 1024 + 3, .min_mb = 8 * 1024 };
	CHECK(rros_plan_build(&p, &r, err, sizeof err) == 0);
	CHECK_STR(p.parts[0].role, "bios_boot");
	CHECK_STR(p.parts[1].fs, "ext4");
	rros_plan_sfdisk(&p, out, sizeof out);
	CHECK(strstr(out, "21686148-6449-6E6F-744E-656564454649") != NULL);

	/* no cabe */
	r.disk_mb = 8 * 1024;
	CHECK(rros_plan_build(&p, &r, err, sizeof err) == -1);
	CHECK(strstr(err, "necesita") != NULL);

	/* cifrado: /boot de 1 GiB sin cifrar y la raíz en LUKS */
	r = (struct rros_plan_req){ .profile = "lite", .uefi = 1, .encrypt = 1, .disk_mb = 20 * 1024, .min_mb = 8 * 1024 };
	CHECK(rros_plan_build(&p, &r, err, sizeof err) == 0);
	CHECK(p.n == 3);
	CHECK_STR(p.parts[1].role, "boot");
	CHECK_STR(p.parts[1].mount, "/boot");
	CHECK(p.parts[1].start_mb == 513 && p.parts[1].size_mb == 1024);
	CHECK(p.parts[2].crypt && p.parts[2].start_mb == 1537);
	rros_plan_table(&p, out, sizeof out);
	CHECK(strstr(out, "root 1537 18942 ext4 / 0 1\n") != NULL);
	/* el cifrado necesita 1 GiB más */
	r.disk_mb = 8 * 1024 + 512 + 2 + 1000;
	CHECK(rros_plan_build(&p, &r, err, sizeof err) == -1);
	CHECK(strstr(err, "/boot") != NULL);

	/* junto a Windows: reutiliza su ESP y usa solo el hueco libre */
	r = (struct rros_plan_req){ .profile = "lite", .uefi = 1, .alongside = 1, .min_mb = 8 * 1024,
				    .free_start_mb = 30000, .free_mb = 20000, .esp_start_mb = 1, .esp_mb = 100 };
	CHECK(rros_plan_build(&p, &r, err, sizeof err) == 0);
	CHECK(p.append && p.n == 2);
	CHECK(p.parts[0].reuse && p.parts[0].start_mb == 1 && p.parts[0].size_mb == 100);
	CHECK(p.parts[1].start_mb == 30000 && p.parts[1].size_mb == 20000);
	rros_plan_sfdisk(&p, out, sizeof out);
	CHECK(strstr(out, "label:") == NULL);           /* --append: no se crea una tabla nueva */
	CHECK(strstr(out, "name=ESP") == NULL);          /* la ESP de Windows no se toca */
	CHECK_STR(out, "start=30000MiB, size=20000MiB, type=L, name=rros-root\n");
	rros_plan_table(&p, out, sizeof out);
	CHECK(strstr(out, "esp 1 100 vfat /boot/efi 1 0\n") == out);

	/* junto a otro sistema sin ESP (UEFI): se crea una en el hueco */
	r.esp_mb = 0;
	r.encrypt = 1;
	CHECK(rros_plan_build(&p, &r, err, sizeof err) == 0);
	CHECK(p.n == 3 && !p.parts[0].reuse && p.parts[0].start_mb == 30000);
	CHECK(p.parts[2].start_mb == 30000 + 512 + 1024 && p.parts[2].size_mb == 20000 - 512 - 1024);
	/* hueco pequeño */
	r.free_mb = 8 * 1024;
	CHECK(rros_plan_build(&p, &r, err, sizeof err) == -1);
	CHECK(strstr(err, "espacio libre") != NULL);

	/* buffer pequeño: no desborda */
	rros_plan_sfdisk(&p, out, 8);
	rros_plan_table(&p, out, 8);
	DONE();
}
