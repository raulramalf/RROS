#ifndef CHECK_H
#define CHECK_H
#include <stdio.h>

static int g_fail, g_total;

#define CHECK(cond) do { g_total++; if (!(cond)) { g_fail++; \
	fprintf(stderr, "  FALLO %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)
#define CHECK_STR(a, b) do { const char *_a = (a), *_b = (b); g_total++; \
	if (!_a || !_b || strcmp(_a, _b)) { g_fail++; \
	fprintf(stderr, "  FALLO %s:%d: '%s' != '%s'\n", __FILE__, __LINE__, \
	_a ? _a : "(null)", _b ? _b : "(null)"); } } while (0)
#define DONE() do { printf("  %d/%d comprobaciones OK\n", g_total - g_fail, g_total); \
	return g_fail ? 1 : 0; } while (0)
#endif
