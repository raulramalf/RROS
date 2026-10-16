CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -std=c11

all: build/rros-hwdetect

build/rros-hwdetect: src/hwdetect/main.c
	@mkdir -p build
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf build

.PHONY: all clean
