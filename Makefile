CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -Wpedantic
LDLIBS ?= -lsodium

.PHONY: build clean

build: build/mic-core

build/mic-core: core/mic_core.c
	mkdir -p build
	$(CC) $(CFLAGS) core/mic_core.c -o build/mic-core $(LDLIBS)

clean:
	rm -rf build audit
