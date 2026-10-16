CC ?= cc
CFLAGS ?= -O2
CFLAGS += -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 \
          -D_DEFAULT_SOURCE -Isrc/lib -Isrc/installer -Isrc/theme
ifdef WERROR
CFLAGS += -Werror
endif

PREFIX ?= /usr
DESTDIR ?=

LIB_SRC := $(wildcard src/lib/*.c)
PLAN_SRC := src/installer/plan.c
THEME_SRC := src/theme/theme.c
BINS := build/rros-hwdetect build/rros-config build/rros-budget build/rros-installer \
        build/rros-theme build/rros-fetch build/rros build/rros-apply

TESTS := $(patsubst tests/%.c,build/tests/%,$(wildcard tests/test_*.c))
SAN := -fsanitize=address,undefined -fno-omit-frame-pointer -g -O1

all: $(BINS)

build/rros-hwdetect: src/hwdetect/main.c $(LIB_SRC)
build/rros-config: src/config/main.c $(LIB_SRC)
build/rros-budget: src/budget/main.c $(LIB_SRC)
build/rros-installer: src/installer/main.c $(PLAN_SRC) $(LIB_SRC)
build/rros-theme: src/theme/main.c $(THEME_SRC) $(LIB_SRC)
build/rros-fetch: src/fetch/main.c $(LIB_SRC)
build/rros: src/rros/main.c $(LIB_SRC)
build/rros-apply: src/apply/main.c $(LIB_SRC)

$(BINS):
	@mkdir -p build
	$(CC) $(CFLAGS) -o $@ $^

build/tests/%: tests/%.c $(LIB_SRC) $(PLAN_SRC) $(THEME_SRC) tests/check.h
	@mkdir -p build/tests
	$(CC) $(CFLAGS) $(SAN) -Itests -o $@ $< $(LIB_SRC) $(PLAN_SRC) $(THEME_SRC)

test: $(TESTS) $(BINS)
	@set -e; for t in $(TESTS); do echo "== $$t"; $$t; done
	@echo "== tests/cli.sh"; bash tests/cli.sh

install: $(BINS)
	install -d $(DESTDIR)$(PREFIX)/bin $(DESTDIR)$(PREFIX)/share/rros/profiles \
	           $(DESTDIR)$(PREFIX)/lib/systemd/system $(DESTDIR)/etc/rros
	install -m 755 $(BINS) $(DESTDIR)$(PREFIX)/bin/
	install -m 644 profiles/*.toml $(DESTDIR)$(PREFIX)/share/rros/profiles/
	install -m 644 dist/systemd/*.service $(DESTDIR)$(PREFIX)/lib/systemd/system/

docs-schema: build/rros-config
	build/rros-config schema --markdown > docs/CUSTOMIZATION.md

clean:
	rm -rf build

.PHONY: all test install docs-schema clean
