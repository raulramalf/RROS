#include <string.h>

#include "check.h"
#include "rros_schema.h"

static int ok(const char *key, const char *val, int quoted)
{
	char err[256];
	const struct rros_key *k = rros_schema_find(key);

	return k && rros_schema_check(k, val, quoted, err, sizeof err) == 0;
}

int main(void)
{
	/* sin claves duplicadas */
	for (size_t i = 0; i < rros_schema_len; i++) {
		for (size_t j = i + 1; j < rros_schema_len; j++)
			CHECK(strcmp(rros_schema[i].key, rros_schema[j].key) != 0);
		if (rros_schema[i].type == RROS_T_ENUM)
			CHECK(rros_schema[i].choices && strchr(rros_schema[i].choices, '|'));
		if (rros_schema[i].type == RROS_T_INT)
			CHECK(rros_schema[i].min <= rros_schema[i].max);
	}
	CHECK(rros_schema_len >= 80);
	CHECK(rros_schema_find("ui.nope") == NULL);

	CHECK(ok("ui.scale", "100", 0));
	CHECK(ok("ui.scale", "50", 0));
	CHECK(ok("ui.scale", "400", 0));
	CHECK(!ok("ui.scale", "49", 0));
	CHECK(!ok("ui.scale", "401", 0));
	CHECK(!ok("ui.scale", "abc", 0));
	CHECK(!ok("ui.scale", "10x", 0));
	CHECK(!ok("ui.scale", "100", 1));
	CHECK(!ok("ui.scale", "", 0));
	CHECK(ok("input.pointer_speed", "-100", 0));

	CHECK(ok("ui.animations", "true", 0));
	CHECK(ok("ui.animations", "false", 0));
	CHECK(!ok("ui.animations", "yes", 0));
	CHECK(!ok("ui.animations", "true", 1));

	CHECK(ok("ui.effects", "advanced", 1));
	CHECK(!ok("ui.effects", "adv", 1));
	CHECK(!ok("ui.effects", "", 1));
	CHECK(ok("display.rotation", "270", 1));
	CHECK(!ok("display.rotation", "45", 1));

	CHECK(ok("ui.theme", "oscuro", 1));
	CHECK(!ok("ui.theme", "mal\x01" "x", 1));
	{
		char big[300];

		memset(big, 'a', sizeof big - 1);
		big[sizeof big - 1] = '\0';
		CHECK(!ok("ui.theme", big, 1));
	}
	CHECK(rros_schema_find("profile.name")->profile_only == 1);
	CHECK(rros_schema_find("ui.theme")->profile_only == 0);
	DONE();
}
