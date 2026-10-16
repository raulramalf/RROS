#include <string.h>

#include "check.h"
#include "plan.h"

int main(void)
{
	struct rros_plan p;
	char err[200], out[512];

	CHECK(rros_plan_make(&p, "pro", 40 * 1024, 1, 22 * 1024, err, sizeof err) == 0);
	CHECK(p.n == 2);
	CHECK_STR(p.parts[0].label, "ESP");
	CHECK(p.parts[0].size_mb == 512);
	CHECK_STR(p.parts[1].fs, "btrfs");
	CHECK(p.parts[1].size_mb == 0);
	rros_plan_sfdisk(&p, out, sizeof out);
	CHECK(strstr(out, "label: gpt\n") == out);
	CHECK(strstr(out, "size=512MiB, type=U") != NULL);
	CHECK(strstr(out, "type=L, name=rros-root") != NULL);

	CHECK(rros_plan_make(&p, "lite", 8 * 1024, 0, 8 * 1024, err, sizeof err) == 0);
	CHECK_STR(p.parts[0].label, "bios_boot");
	CHECK_STR(p.parts[1].fs, "ext4");
	rros_plan_sfdisk(&p, out, sizeof out);
	CHECK(strstr(out, "21686148-6449-6E6F-744E-656564454649") != NULL);

	CHECK(rros_plan_make(&p, "lite", 8 * 1024 - 1, 1, 8 * 1024, err, sizeof err) == -1);
	CHECK(strstr(err, "necesita") != NULL);
	/* buffer pequeño: no desborda */
	rros_plan_sfdisk(&p, out, 8);
	DONE();
}
