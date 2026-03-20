#include <chip8.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

const uint8_t chip8_fontset[80] = {
    // 0
    0xF0,
    0x90,
    0x90,
    0x90,
    0xF0,
    // 1
    0x20,
    0x60,
    0x20,
    0x20,
    0x70,
    // 2
    0xF0,
    0x10,
    0xF0,
    0x80,
    0xF0,
    // 3
    0xF0,
    0x10,
    0xF0,
    0x10,
    0xF0,
    // 4
    0x90,
    0x90,
    0xF0,
    0x10,
    0x10,
    // 5
    0xF0,
    0x80,
    0xF0,
    0x10,
    0xF0,
    // 6
    0xF0,
    0x80,
    0xF0,
    0x90,
    0xF0,
    // 7
    0xF0,
    0x10,
    0x20,
    0x40,
    0x40,
    // 8
    0xF0,
    0x90,
    0xF0,
    0x90,
    0xF0,
    // 9
    0xF0,
    0x90,
    0xF0,
    0x10,
    0xF0,
    // A
    0xF0,
    0x90,
    0xF0,
    0x90,
    0x90,
    // B
    0xE0,
    0x90,
    0xE0,
    0x90,
    0xE0,
    // C
    0xF0,
    0x80,
    0x80,
    0x80,
    0xF0,
    // D
    0xE0,
    0x90,
    0x90,
    0x90,
    0xE0,
    // E
    0xF0,
    0x80,
    0xF0,
    0x80,
    0xF0,
    // F
    0xF0,
    0x80,
    0xF0,
    0x80,
    0x80,
};

void chip8_init(Chip8* chip) {
  // Zero mem and set PC to start address
  memset(chip, 0, sizeof(Chip8));
  chip->pc = PC_START_ADDRESS;
  chip->sp = 0;

  // Load fontset in the start of memory
  memcpy(chip->memory, chip8_fontset, sizeof(chip8_fontset));
}

int chip_8_load_rom(Chip8* chip, const char* filename) {}

void chip8_start(Chip8* chip) {}

void handle_input(Chip8* chip) {}

void chip8_step(Chip8* chip) {
  uint16_t instruction = ((uint16_t)chip->memory[chip->pc] << 8) | chip->memory[chip->pc + 1];
  uint16_t opcode = instruction & 0xF000;

  uint16_t nnn = (instruction & 0x0FFF);
  uint8_t x = (instruction & 0x0F00) >> 8;
  uint8_t y = (instruction & 0x00F0) >> 4;
  uint8_t kk = (instruction & 0x00FF);
  uint8_t n = (instruction & 0x000F);

  chip->pc += 2;

  switch (opcode) {
      // System
    case 0x0000: {
      switch (instruction & 0x00FF) {
          // Clear
        case 0x00E0: {
          memset(chip->display, 0, sizeof(chip->display));
        } break;
          // Return
        case 0x00EE: {
          chip->sp--;
          chip->pc = chip->stack[chip->sp];
        } break;
      }
    } break;
      // Jump
    case 0x1000: {
      chip->pc = nnn;
    } break;
      // Call subroutine
    case 0x2000: {
      chip->stack[chip->sp] = chip->pc;
      chip->sp++;
      chip->pc = nnn;
    } break;
      // Skip if Vx == kk
    case 0x3000: {
      if (chip->registers[x] == kk) {
        chip->pc += 2;
      }
    } break;
      // Skip if Vx != kk
    case 0x4000: {
      if (chip->registers[x] != kk) {
        chip->pc += 2;
      }
    } break;
      // Skip if Vx == Vy
    case 0x5000: {
      if (chip->registers[x] == chip->registers[y]) {
        chip->pc += 2;
      }
    } break;
      // Load Vx
    case 0x6000: {
      chip->registers[x] = kk;
    } break;
      // Add Vx
    case 0x7000: {
      chip->registers[x] += kk;
    } break;
      // Set Vx, Vy
    case 0x8000: {
      switch (n) {
          // Vx = Vy
        case 0x00: {
          chip->registers[x] = chip->registers[y];
        } break;
          // Vx = Vx | Vy
        case 0x01: {
          chip->registers[x] |= chip->registers[y];
        } break;
          // Vx = Vx & Vy
        case 0x02: {
          chip->registers[x] &= chip->registers[y];
        } break;
          // Vx = Vx XOR Vy
        case 0x03: {
          chip->registers[x] ^= chip->registers[y];
        } break;
          // Vx = Vx + Vy, Vf = carry
        case 0x04: {
          uint16_t sum = (uint16_t)chip->registers[x] + (uint16_t)chip->registers[y];
          if (sum > 255) {
            chip->registers[0xF] = 1;
          } else {
            chip->registers[0xF] = 0;
          }
          chip->registers[x] = (uint8_t)(sum & 0xFF);
        } break;
          // Vx = Vx - Vy, Vf = not borrow
        case 0x05: {
          if (chip->registers[x] >= chip->registers[y]) {
            chip->registers[0xF] = 1;
          } else {
            chip->registers[0xF] = 0;
          }
          chip->registers[x] -= chip->registers[y];
        } break;
          // SHR Vx {, Vy}
        case 0x06: {
          chip->registers[0xF] = (chip->registers[x] & 0x01);
          chip->registers[x] >>= 1;
        } break;
          // SUBN Vx, Vy, Vf = not borrow
        case 0x07: {
          if (chip->registers[y] >= chip->registers[x]) {
            chip->registers[0xF] = 1;
          } else {
            chip->registers[0xF] = 0;
          }
          chip->registers[x] = chip->registers[y] - chip->registers[x];
        } break;
          // SHL Vx {, Vy}
        case 0x0E: {
          chip->registers[0xF] = (chip->registers[x] & 0x80) >> 7;
          chip->registers[x] <<= 1;
        } break;
      }
    } break;
      // Skip if Vx != Vy
    case 0x9000: {
      if (chip->registers[x] != chip->registers[y]) {
        chip->pc += 2;
      }
    } break;
      // Set index register
    case 0xA000: {
      chip->index = nnn;
    } break;
      // Jump nnn + V0
    case 0xB000: {
      chip->pc = nnn + chip->registers[0x0];
    } break;
      // Vx = rand byte & kk
    case 0xC000: {
      uint8_t random_val = rand() & 0xFF;
      chip->registers[x] = random_val & kk;
    } break;
      // Display n-byte sprite starting at index at (Vx, Vy)
    case 0xD000: {
      uint8_t x_pos = chip->registers[x] % 64;
      uint8_t y_pos = chip->registers[y] % 32;
      chip->registers[0xF] = 0;

      for (int row = 0; row < n; row++) {
        uint8_t sprite_byte = chip->memory[chip->index + row];

        for (int col = 0; col < 8; col++) {
          uint8_t sprite_pixel = (sprite_byte >> (7 - col)) & 0x01;

          // Sprites are XORd onto screen, only do something if 1 (on), 0 is no-op
          if (sprite_pixel == 1) {
            int screen_x = (x_pos + col) % 64;
            int screen_y = (y_pos + row) % 32;
            int display_index = screen_y * 64 + screen_x;

            // Set Vf if collision
            if (chip->display[display_index] == 1) {
              chip->registers[0xF] = 1;
            }

            // XOR display pixel
            chip->display[display_index] ^= 1;
          }
        }
      }
    } break;
      // Skip on key with value of Vx
    case 0xE000: {
      uint8_t key = chip->registers[x] & 0x0F;
      switch (kk) {
          // If key pressed
        case 0x9E: {
          if (chip->keypad[key] == 1) {
            chip->pc += 2;
          }
        } break;
          // If key not pressed
        case 0xA1: {
          if (chip->keypad[key] != 1) {
            chip->pc += 2;
          }
        } break;
      }
    } break;
      // LD Vx
    case 0xF000: {
      switch (kk) {
          // Vx = DT
        case 0x07: {
          chip->registers[x] = chip->delay_timer;
        } break;
          // Loop until key pressed, store key in Vx
        case 0x0A: {
          bool key_pressed = false;

          for (int i = 0; i < 16; i++) {
            if (chip->keypad[i] == 1) {
              chip->registers[x] = i;
              key_pressed = true;
              break;
            }
          }

          if (!key_pressed) {
            chip->pc -= 2;
          }
        } break;
          // DT = Vx
        case 0x15: {
          chip->delay_timer = chip->registers[x];
        } break;
          // ST = Vx
        case 0x18: {
          chip->sound_timer = chip->registers[x];
        } break;
          // I = I + Vx
        case 0x1E: {
          chip->index += chip->registers[x];
        } break;
          // I = start of digit Vx
        case 0x29: {
          uint8_t digit = chip->registers[x] & 0x0F;
          chip->index = digit * 5;
        } break;
          // BCD of Vx in I, I+1 and I+2
        case 0x33: {
          uint8_t reg_x = chip->registers[x];
          chip->memory[chip->index] = reg_x / 100;
          chip->memory[chip->index + 1] = (reg_x / 10) % 10;
          chip->memory[chip->index + 2] = (reg_x % 10);
        } break;
          // Store V0 through Vx in memory starting at I
        case 0x55: {
          memcpy(&chip->memory[chip->index], chip->registers, x + 1);
        } break;
          // Read V0 through Vx from memory starting at I
        case 0x65: {
          memcpy(chip->registers, &chip->memory[chip->index], x + 1);
        } break;
      }
    } break;
  }
}