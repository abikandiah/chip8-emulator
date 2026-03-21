#pragma once

#include <stdbool.h>
#include <stdint.h>

#define CHIP8_MEMORY_SIZE 4096
#define PC_START_ADDRESS 0x200
#define CPU_FREQ 500
#define KEYPAD_SIZE 16

typedef struct {
  uint8_t memory[4096];
  uint8_t registers[16];
  uint16_t index;
  uint16_t pc;
  uint16_t stack[16];
  uint8_t sp;
  uint8_t delay_timer;
  uint8_t sound_timer;
  uint8_t keypad[KEYPAD_SIZE];
  uint32_t display[64 * 32];
  bool draw_flag;
} Chip8;

void chip8_init(Chip8* chip);
int chip8_load_rom(Chip8* chip, const char* filename);
void chip8_step(Chip8* chip);
void chip8_decrement_timers(Chip8* chip);
