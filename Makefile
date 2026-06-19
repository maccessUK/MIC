CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -Wpedantic
LDLIBS ?= -lsodium

.PHONY: build clean

build: build/mic-runtime

build/mic-runtime: runtime/mic_runtime.c
	mkdir -p build
	$(CC) $(CFLAGS) runtime/mic_runtime.c -o build/mic-runtime $(LDLIBS)

clean:
	rm -rf build audit
