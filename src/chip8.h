#pragma once

#include <stdint.h>

#define PC_START_ADDRESS 0x200;

typedef struct {
  uint8_t memory[4096];
  uint8_t registers[16];
  uint16_t index;
  uint16_t pc;
  uint16_t stack[16];
  uint8_t sp;
  uint8_t delay_timer;
  uint8_t sound_timer;
  uint32_t display[64 * 32];
  uint8_t keypad[16];
} Chip8;

extern const uint8_t chip8_fontset[80];

void chip8_init(Chip8* chip);
int chip_8_load_rom(Chip8* chip, const char* filename);
void chip8_start(Chip8* chip);
