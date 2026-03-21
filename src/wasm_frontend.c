#include <emscripten.h>
#include <stdlib.h>
#include <string.h>

#include "chip8.h"

static Chip8* chip;
static int last_rom_size = 0;

// Static staging buffer for ROM upload — JS writes here directly,
// avoiding Module._malloc/HEAPU8 stale-buffer issues with ALLOW_MEMORY_GROWTH.
static uint8_t rom_staging[CHIP8_MEMORY_SIZE - PC_START_ADDRESS];

static void wasm_reset(void) { chip8_init(chip); }

EMSCRIPTEN_KEEPALIVE
uint8_t* wasm_get_rom_buffer(void) { return rom_staging; }

EMSCRIPTEN_KEEPALIVE
int wasm_load_rom_from_buffer(int size) {
  if (size <= 0 || size > (CHIP8_MEMORY_SIZE - PC_START_ADDRESS)) return -1;
  last_rom_size = size;
  wasm_reset();
  memcpy(&chip->memory[PC_START_ADDRESS], rom_staging, size);
  return 0;
}

EMSCRIPTEN_KEEPALIVE
int wasm_reset_rom(void) {
  if (last_rom_size <= 0) return -1;
  wasm_reset();
  memcpy(&chip->memory[PC_START_ADDRESS], rom_staging, last_rom_size);
  return 0;
}

EMSCRIPTEN_KEEPALIVE
void wasm_set_paused(int p) {
  if (p) emscripten_pause_main_loop();
  else emscripten_resume_main_loop();
}

EMSCRIPTEN_KEEPALIVE
int wasm_load_rom(uint8_t* data, int size) {
  if (size <= 0 || size > (CHIP8_MEMORY_SIZE - PC_START_ADDRESS)) return -1;
  wasm_reset();
  memcpy(&chip->memory[PC_START_ADDRESS], data, size);
  return 0;
}

EMSCRIPTEN_KEEPALIVE
void wasm_set_key(int key, int value) {
  if (key >= 0 && key < KEYPAD_SIZE) chip->keypad[key] = (uint8_t)value;
}

EMSCRIPTEN_KEEPALIVE
uint32_t* wasm_get_display(void) { return chip->display; }

static void wasm_frame(void) {
  if (!chip || last_rom_size <= 0) return;

  for (int i = 0; i < CPU_FREQ / 60; i++) {
    chip8_step(chip);
  }

  // wasm_frame is called at exactly 60fps, so decrement once per frame
  chip8_decrement_timers(chip);

  if (chip->draw_flag) {
    EM_ASM(renderFrame(););
    chip->draw_flag = false;
  }
}

int main(void) {
  chip = malloc(sizeof(Chip8));
  if (chip == NULL) return 1;
  chip8_init(chip);
  emscripten_set_main_loop(wasm_frame, 60, 0);
  return 0;
}
