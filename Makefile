# Variables
CC = gcc
EMCC = emcc
CFLAGS = -Isrc -Wall -Wextra -std=c11 -O2
EMFLAGS = -Isrc -std=c11 -O2 \
          -s WASM=1 \
          -s EXPORTED_FUNCTIONS='["_wasm_load_rom","_wasm_set_key","_wasm_get_display","_malloc","_free"]' \
          -s EXPORTED_RUNTIME_METHODS='["HEAPU8","HEAPU32"]' \
          -s ALLOW_MEMORY_GROWTH=1

TARGET = chip8
WASM_TARGET = web/chip8.js
SRCDIR = src

SRCS = $(SRCDIR)/main.c $(SRCDIR)/chip8.c $(SRCDIR)/terminal.c
OBJS = $(SRCS:.c=.o)

WASM_SRCS = $(SRCDIR)/wasm_frontend.c $(SRCDIR)/chip8.c
WASM_ROMS = web/roms/glitchGhost.ch8

.PHONY: all wasm clean

# Default: terminal build
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Wasm build — produces full deployable web/ directory
wasm: $(WASM_TARGET) $(WASM_ROMS)

$(WASM_TARGET): $(WASM_SRCS) $(SRCDIR)/chip8.h
	$(EMCC) $(EMFLAGS) $(WASM_SRCS) -o $(WASM_TARGET)

web/roms/%: roms/%
	mkdir -p web/roms
	cp $< $@

# Clean up
clean:
	rm -f $(OBJS) $(TARGET) $(WASM_TARGET) web/chip8.wasm
	rm -rf web/roms
