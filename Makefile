CC ?= cc
CFLAGS ?= -O2
CFLAGS += -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 \
          -D_DEFAULT_SOURCE -Isrc/lib -Isrc/installer -Isrc/theme -Isrc/settings
ifdef WERROR
CFLAGS += -Werror
endif

PREFIX ?= /usr
DESTDIR ?=

LIB_SRC := $(wildcard src/lib/*.c)
PLAN_SRC := src/installer/plan.c
THEME_SRC := src/theme/theme.c src/theme/palettes.c
BINS := build/rros-hwdetect build/rros-config build/rros-budget build/rros-installer \
        build/rros-theme build/rros-fetch build/rros build/rros-apply build/rros-settings build/rros-apps \
        build/rros-security build/rros-devices build/rros-new

TESTS := $(patsubst tests/%.c,build/tests/%,$(wildcard tests/test_*.c))
SAN := -fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer -g -O1

all: $(BINS)

build/rros-hwdetect: src/hwdetect/main.c $(LIB_SRC)
build/rros-config: src/config/main.c $(LIB_SRC)
build/rros-budget: src/budget/main.c $(LIB_SRC)
build/rros-installer: src/installer/main.c $(PLAN_SRC) $(LIB_SRC)
build/rros-theme: src/theme/main.c $(THEME_SRC) $(LIB_SRC)
build/rros-fetch: src/fetch/main.c $(THEME_SRC) $(LIB_SRC)
build/rros: src/rros/main.c $(LIB_SRC)
build/rros-apply: src/apply/main.c $(LIB_SRC)
build/rros-apps: src/apps/main.c $(LIB_SRC)
build/rros-security: src/security/main.c $(LIB_SRC)
build/rros-devices: src/devices/main.c $(LIB_SRC)
build/rros-new: src/new/main.c $(LIB_SRC)
SETTINGS_SRC := $(wildcard src/settings/*.c)
build/rros-settings: $(SETTINGS_SRC) $(THEME_SRC) $(LIB_SRC)

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
	install -d $(DESTDIR)$(PREFIX)/share/rros/settings $(DESTDIR)$(PREFIX)/share/rros/apps
	install -m 644 apps/catalog.toml $(DESTDIR)$(PREFIX)/share/rros/apps/catalog.toml
	install -m 644 gui/index.html $(DESTDIR)$(PREFIX)/share/rros/settings/index.html
	install -m 644 dist/systemd/*.service $(DESTDIR)$(PREFIX)/lib/systemd/system/
	install -d $(DESTDIR)$(PREFIX)/share/rros/templates
	cp -R templates/. $(DESTDIR)$(PREFIX)/share/rros/templates/

docs-schema: build/rros-config
	build/rros-config schema --markdown > docs/CUSTOMIZATION.md

clean:
	rm -rf build

.PHONY: all test install docs-schema clean
