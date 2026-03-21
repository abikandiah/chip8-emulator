#include <emscripten.h>
#include <stdlib.h>
#include <string.h>

#include "chip8.h"

static Chip8* chip;

static void wasm_reset(void) {
  chip8_init(chip);
}

EMSCRIPTEN_KEEPALIVE
int wasm_load_rom(uint8_t* data, int size) {
  if (size > (CHIP8_MEMORY_SIZE - PC_START_ADDRESS)) return -1;
  wasm_reset();
  memcpy(&chip->memory[PC_START_ADDRESS], data, size);
  return 0;
}

EMSCRIPTEN_KEEPALIVE
void wasm_set_key(int key, int value) {
  if (key >= 0 && key < KEYPAD_SIZE)
    chip->keypad[key] = (uint8_t)value;
}

EMSCRIPTEN_KEEPALIVE
uint32_t* wasm_get_display(void) {
  return chip->display;
}

static void wasm_frame(void) {
  if (!chip) return;

  for (int i = 0; i < CPU_FREQ / 60; i++) {
    chip8_step(chip);
  }

  // wasm_frame is called at exactly 60fps, so decrement once per frame
  chip8_decrement_timers(chip);

  if (chip->draw_flag) {
    EM_ASM( renderFrame(); );
    chip->draw_flag = false;
  }
}

int main(void) {
  chip = malloc(sizeof(Chip8));
  if (chip == NULL) return 1;
  chip8_init(chip);
  emscripten_set_main_loop(wasm_frame, 60, 1);
  return 0;
}
