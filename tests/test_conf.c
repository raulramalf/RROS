#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "check.h"
#include "rros_conf.h"

static void test_parse(void)
{
	struct rros_conf c;
	int line = 0;
	const char *txt =
		"top = 1  # comentario\n"
		"\n"
		"[ui]\n"
		"theme = \"oscuro # no es comentario\"\n"
		"scale = 150\n"
		"animations = false\n"
		"  # solo comentario\n"
		"[a.b]\r\n"
		"k = \"con \\\"comillas\\\" y \\\\ y \\n salto\"\n";

	rros_conf_init(&c);
	CHECK(rros_conf_parse(&c, txt, &line) == 0);
	CHECK_STR(rros_conf_get(&c, "top"), "1");
	CHECK_STR(rros_conf_get(&c, "ui.theme"), "oscuro # no es comentario");
	CHECK(rros_conf_get_long(&c, "ui.scale", -1) == 150);
	CHECK(rros_conf_get_bool(&c, "ui.animations", 1) == 0);
	CHECK_STR(rros_conf_get(&c, "a.b.k"), "con \"comillas\" y \\ y \n salto");
	CHECK(rros_conf_get(&c, "nope") == NULL);
	CHECK(rros_conf_get_long(&c, "ui.theme", 7) == 7);
	CHECK(rros_conf_get_bool(&c, "ui.scale", 1) == 1);
	rros_conf_free(&c);
}

static void test_errors(void)
{
	const char *bad[] = {
		"sin igual\n", "k =\n", "[abierta\n", "[bad sec]\n", "k = \"sin cerrar\n",
		"k = \"a\" basura\n", "k = dos palabras\n", "k.x = 1\n", "k = \"\\q\"\n", "= 1\n",
	};

	for (size_t i = 0; i < sizeof bad / sizeof *bad; i++) {
		struct rros_conf c;
		int line = 0;

		rros_conf_init(&c);
		CHECK(rros_conf_parse(&c, bad[i], &line) == -2);
		CHECK(line == 1);
		rros_conf_free(&c);
	}
	{
		struct rros_conf c;
		int line = 0;

		rros_conf_init(&c);
		CHECK(rros_conf_parse(&c, "a = 1\nb = 2\nmal\n", &line) == -2);
		CHECK(line == 3);
		rros_conf_free(&c);
	}
}

static void test_set_unset(void)
{
	struct rros_conf c;

	rros_conf_init(&c);
	for (int i = 0; i < 100; i++) {
		char k[32];

		snprintf(k, sizeof k, "s.k%d", i);
		CHECK(rros_conf_set(&c, k, "v", 1) == 0);
	}
	CHECK(c.n == 100);
	CHECK(rros_conf_set(&c, "s.k5", "nuevo", 1) == 0);
	CHECK(c.n == 100);
	CHECK_STR(rros_conf_get(&c, "s.k5"), "nuevo");
	CHECK(rros_conf_unset(&c, "s.k5") == 1);
	CHECK(rros_conf_unset(&c, "s.k5") == 0);
	CHECK(c.n == 99);
	CHECK(rros_conf_get(&c, "s.k5") == NULL);
	rros_conf_free(&c);
}

static int find_quoted_equal(const struct rros_conf *c, const struct rros_kv *want)
{
	for (size_t i = 0; i < c->n; i++)
		if (!strcmp(c->items[i].key, want->key))
			return c->items[i].quoted == want->quoted;
	return 0;
}

static void test_roundtrip(void)
{
	char dir[] = "/tmp/rros-test-XXXXXX";
	char path[128];
	struct rros_conf a, b;
	int line;

	CHECK(mkdtemp(dir) != NULL);
	snprintf(path, sizeof path, "%s/c.toml", dir);
	rros_conf_init(&a);
	rros_conf_init(&b);
	rros_conf_set(&a, "top", "x", 0);
	rros_conf_set(&a, "ui.theme", "a \"b\" \\ c\nd", 1);
	rros_conf_set(&a, "ui.scale", "120", 0);
	rros_conf_set(&a, "ui.sub.deep", "true", 0);
	rros_conf_set(&a, "power.mode", "saver", 1);
	CHECK(rros_conf_save(&a, path) == 0);
	CHECK(rros_conf_load(&b, path, &line) == 0);
	CHECK(a.n == b.n);
	for (size_t i = 0; i < a.n; i++) {
		CHECK_STR(rros_conf_get(&b, a.items[i].key), a.items[i].val);
		CHECK(find_quoted_equal(&b, &a.items[i]));
	}
	CHECK(rros_conf_load(&b, "/no/existe", &line) == -1);
	rros_conf_free(&a);
	rros_conf_free(&b);
	unlink(path);
	rmdir(dir);
}

int main(void)
{
	test_parse();
	test_errors();
	test_set_unset();
	test_roundtrip();
	DONE();
}
