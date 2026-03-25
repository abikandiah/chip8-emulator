# CHIP-8 Emulator

A fully-featured CHIP-8 virtual machine emulator written in C, deployable as both a native terminal application and a browser-based experience via WebAssembly.

**[chip8.akandiah.ca ↗](https://chip8.akandiah.ca)**

`C` `WebAssembly` `Emscripten` `HTML` `CSS` `JavaScript`

---

## Overview

This is a software implementation of the CHIP-8 virtual machine — an interpreted platform originally designed in 1977 for early home microcomputers. It executes programs (ROMs) written for the CHIP-8 platform, including classic games like Snake and Br8kout.

I built this around a **shared core, multiple frontends** pattern. The virtual machine — CPU, memory, display buffer, and keypad — is implemented once in portable C11 with no external dependencies, and adapted by two thin deployment layers:

- **Terminal** — Renders the display as UTF-8 block characters and reads keyboard input via raw termios, running natively on Linux.
- **Browser** — Compiled to WebAssembly via Emscripten, driving the CPU loop at 60 FPS and rendering to an HTML5 canvas. Playable in the browser with no installation.

---

## CHIP-8 Specification

CHIP-8 is an interpreted programming language and virtual machine specification first published in 1977 by Joseph Weisbecker. It was designed to make game development easier for early microcomputers like the COSMAC VIP and Telmac 1800.

Despite its age and simplicity, CHIP-8 remains a popular target for learning emulator development — it has a small, well-documented instruction set that is complex enough to be meaningful but small enough to implement in a weekend.

| Property | Value |
|---|---|
| Memory | 4,096 bytes (4 KB) |
| Registers | 16 × 8-bit (V0–VF) |
| Display | 64 × 32 pixels, monochrome |
| Clock speed | ~500 Hz |
| Input | 16-key hexadecimal keypad |
| Sound | Single-tone buzzer |
| Stack depth | 16 levels |
| Instruction size | 2 bytes (fixed-width) |

CHIP-8 programs are stored as binary files (`.ch8` ROMs) and loaded starting at memory address `0x200`. The first 512 bytes are reserved for the interpreter itself, including built-in font sprites for hexadecimal digits.

---

## System Architecture

The five source files each own a distinct responsibility:

- `chip8.c` / `chip8.h` — the virtual machine. Pure logic with no platform dependencies.
- `main.c` — the entry point for the native build. Parses arguments, allocates the VM, and hands off to the terminal frontend.
- `terminal.c` — the POSIX/Linux frontend. Handles rendering to stdout and keyboard input via termios.
- `wasm_frontend.c` — the WebAssembly bridge. Exports C functions to JavaScript and runs the CPU loop via Emscripten's scheduler.
- `web/index.html` — the browser UI. JavaScript handles canvas rendering, input events, ROM loading, and UI controls.

### Core Emulator Design

All virtual machine state lives in a single struct. The struct itself contains no internal heap allocations — every array is inline — so it can be allocated once and passed around by pointer with no further memory management.

```c
typedef struct {
  uint8_t  memory[4096];       // 4 KB address space
  uint8_t  registers[16];      // V0–VF general-purpose registers
  uint16_t index;              // Index register (memory addressing)
  uint16_t pc;                 // Program counter
  uint16_t stack[16];          // Call stack (16 subroutine levels)
  uint8_t  sp;                 // Stack pointer
  uint8_t  delay_timer;        // Countdown timer at 60 Hz
  uint8_t  sound_timer;        // Buzzer timer at 60 Hz
  uint8_t  keypad[16];         // Current keypad state (1=pressed)
  uint32_t display[64 * 32];   // Pixel buffer (1=on, 0=off)
  bool     draw_flag;          // Dirty bit: display needs redraw
} Chip8;
```

### Instruction Execution

Each call to `chip8_step()` performs one complete fetch-decode-execute cycle: read 2 bytes at `memory[pc]`, combine into a 16-bit opcode, advance `pc` by 2, then switch on the top nibble to dispatch. All 34 standard CHIP-8 instructions are implemented.

### Sprite Rendering & Collision Detection

The `0xDXYN` opcode draws sprites using XOR logic — drawing the same sprite twice erases it, which games rely on for animation. Each drawn pixel is XOR'd with the display buffer; if a pixel was already on, the collision flag (`VF`) is set to 1.

### Timing

The emulator runs at two distinct frequencies:

- **CPU:** 500 Hz — one instruction per 2,000 microseconds.
- **Timers:** 60 Hz — `delay_timer` and `sound_timer` decrement once per frame.

In the terminal, precise timing is achieved with `nanosleep()`. In the browser, Emscripten's `emscripten_set_main_loop()` fires at 60 FPS, and the CPU executes 8 instructions per frame to maintain the correct frequency ratio.

---

## Terminal Interface

The terminal interface renders the 64×32 display using UTF-8 block characters directly in the terminal window, with real-time keyboard input.

### Rendering

Each pixel is rendered as a pair of full-block characters — ON pixels use `██` and OFF pixels use two spaces. The cursor is repositioned to the top-left with an ANSI escape before each frame, so the display updates in-place without flickering or scrolling.

### Input

The terminal is put into raw mode via POSIX termios: line buffering is disabled, echo is disabled, and reads are non-blocking. Keyboard keys map to the 16-key CHIP-8 hexadecimal keypad:

```
Keyboard  →  CHIP-8 Keypad
─────────────────────────────
1 2 3 4   →  1 2 3 C
Q W E R   →  4 5 6 D
A S D F   →  7 8 9 E
Z X C V   →  A 0 B F
```

---

## WebAssembly Interface

The WASM frontend is a thin bridge layer that exports C functions to JavaScript and drives the emulator using Emscripten's main loop mechanism. These functions are callable from JavaScript as `Module._functionName()`:

| Function | Purpose |
|---|---|
| `wasm_get_rom_buffer()` | Returns a pointer to the ROM staging buffer in WASM memory |
| `wasm_load_rom_from_buffer(size)` | Loads ROM from the staging buffer, resets VM |
| `wasm_reset_rom()` | Re-initializes the VM and reloads the current ROM |
| `wasm_set_paused(p)` | Pauses or resumes the Emscripten main loop |
| `wasm_set_key(key, value)` | Sets the state of a keypad key (1=pressed, 0=released) |
| `wasm_get_display()` | Returns a pointer to the display pixel buffer in WASM memory |

### Memory Sharing — The Staging Buffer Pattern

A key challenge with WASM is safely sharing data between C and JavaScript. When `ALLOW_MEMORY_GROWTH=1` is enabled (required for ROM loading), the WASM memory buffer can be reallocated, invalidating any JavaScript references to raw pointers.

The solution is a static staging buffer in C. JavaScript writes ROM bytes into the buffer before calling any function that could trigger memory growth, making the approach safe regardless of the WASM memory layout.

```c
static uint8_t rom_staging[CHIP8_MEMORY_SIZE - PC_START_ADDRESS];  // 3,584 bytes

uint8_t* wasm_get_rom_buffer(void) {
    return rom_staging;
}

int wasm_load_rom_from_buffer(int size) {
    // Copy from staging buffer → chip8.memory[0x200]
    memcpy(&chip->memory[PC_START_ADDRESS], rom_staging, size);
    return 0;
}
```

---

## Web Frontend

The browser UI is a single self-contained HTML file with no external dependencies beyond the Emscripten-generated JS loader. It includes:

- **Canvas** — 64×32 display scaled up to fit the viewport (max 640px wide), using `image-rendering: pixelated` for crisp scaling.
- **ROM Selector** — dropdown to pick one of 3 bundled ROMs (Snake, Rock Paper Scissors, Br8kout).
- **File Upload** — allows loading any custom `.ch8` ROM from the user's machine.
- **Controls** — Reset and Pause/Resume buttons.
- **Key Map Reference** — visual display of the keyboard-to-keypad mapping.

The display pointer is read directly from WASM linear memory via `HEAPU32`, then converted pixel-by-pixel into an `ImageData` object and stamped onto the canvas with `putImageData`. This reads the pixel buffer straight out of WASM's address space without an intermediate JavaScript array allocation.

```javascript
function renderFrame() {
    const ptr = Module._wasm_get_display();
    const display = new Uint32Array(Module.HEAPU32.buffer, ptr, 64 * 32);

    const imageData = ctx.createImageData(64, 32);
    for (let i = 0; i < 64 * 32; i++) {
        const on = display[i] ? 255 : 0;
        imageData.data[i * 4 + 0] = on;  // R
        imageData.data[i * 4 + 1] = on;  // G
        imageData.data[i * 4 + 2] = on;  // B
        imageData.data[i * 4 + 3] = 255; // A
    }
    ctx.putImageData(imageData, 0, 0);
}
```

---

## Build System

The Makefile provides two build targets:

### Native Binary

```bash
# Compile with GCC
make
# Produces: ./chip8 (Linux executable)

# Run a ROM
./chip8 roms/snake.ch8
```

### WebAssembly

```bash
# Compile with Emscripten
make wasm
# Produces: web/chip8.js, web/chip8.wasm
```

WebAssembly requires specific HTTP headers to run in a secure browser context. The deployment includes a `_headers` file that configures `Cross-Origin-Opener-Policy` and `Cross-Origin-Embedder-Policy` for static hosting.
