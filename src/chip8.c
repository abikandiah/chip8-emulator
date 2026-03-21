#define _POSIX_C_SOURCE 199309L
#include <chip8.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

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

struct termios original_termios;

// Restore the terminal to how it was before
void disable_raw_mode() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios); }

void enable_raw_mode() {
  // Get current terminal attributes
  tcgetattr(STDIN_FILENO, &original_termios);

  // Register 'disable' to run on exit
  atexit(disable_raw_mode);

  struct termios raw = original_termios;

  // Flip the bits to disable "Echo" and "Canonical" mode
  raw.c_lflag &= ~(ICANON | ECHO);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 0;

  // Set the new attributes
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void chip8_init(Chip8* chip) {
  // Zero mem and set PC to start address
  memset(chip, 0, sizeof(Chip8));
  chip->pc = PC_START_ADDRESS;
  chip->sp = 0;

  // Load fontset in the start of memory
  memcpy(chip->memory, chip8_fontset, sizeof(chip8_fontset));

  // Buffer stdout for terminal rendering
  setvbuf(stdout, NULL, _IOFBF, 64 * 32 * 4);
  // Seed random
  srand(time(NULL));
}

void chip8_start(Chip8* chip) {
  int cycle_delay_ns = 1000000000 / CPU_FREQ;
  int timer_counter = 0;

  while (1) {
    // Track elapsed time to more precisely hit targeted CPU_FREQ
    struct timespec start, end, sleep_time;
    clock_gettime(CLOCK_MONOTONIC, &start);

    chip8_handle_termnal_input(chip);
    chip8_step(chip);

    // Timers limited to 60Hz
    timer_counter += 60;
    if (timer_counter >= CPU_FREQ) {
      chip8_decrement_timers(chip);
      timer_counter -= 60;
    }

    if (chip->draw_flag) {
      chip8_render_terminal(chip);
      chip->draw_flag = false;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    long elapsed_ns = (end.tv_sec - start.tv_sec) * 1000000000L + (end.tv_nsec - start.tv_nsec);

    // Only sleep for remaining time
    if (elapsed_ns < cycle_delay_ns) {
      sleep_time.tv_sec = 0;
      sleep_time.tv_nsec = cycle_delay_ns - elapsed_ns;
      nanosleep(&sleep_time, NULL);
    }
  }
}

int chip_8_load_rom(Chip8* chip, const char* filename) {
  FILE* file = fopen(filename, "rb");
  if (!file) {
    perror("Error opening ROM");
    return -1;
  }

  // Determine file size
  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  rewind(file);

  // Max size is 4096 - 512 = 3584 bytes
  if (size > (4096 - 512)) {
    fprintf(stderr, "ROM is too large to fit in memory.\n");
    fclose(file);
    return -1;
  }

  // Read file into memory
  size_t result = fread(&chip->memory[PC_START_ADDRESS], 1, size, file);
  if (result != (size_t)size) {
    fprintf(stderr, "Error reading ROM data.\n");
    fclose(file);
    return -1;
  }

  fclose(file);
  return 0;
}

void chip8_render_terminal(Chip8* chip) {
  // Move cursor to top-left (avoids flickering)
  printf("\033[H");

  for (int y = 0; y < 32; y++) {
    for (int x = 0; x < 64; x++) {
      if (chip->display[y * 64 + x]) {
        printf("██");
        // printf("##");
      } else {
        printf("  ");
      }
    }
    printf("\n");
  }
}

void chip8_handle_termnal_input(Chip8* chip) {
  // Reset all keys
  memset(chip->keypad, 0, sizeof(chip->keypad));

  char c = 0;
  while (read(STDIN_FILENO, &c, 1) == 1) {
    switch (c) {
      case '1':
        chip->keypad[0x1] = 1;
        break;
      case '2':
        chip->keypad[0x2] = 1;
        break;
      case '3':
        chip->keypad[0x3] = 1;
        break;
      case '4':
        chip->keypad[0xC] = 1;
        break;
      case 'q':
      case 'Q':
        chip->keypad[0x4] = 1;
        break;
      case 'w':
      case 'W':
        chip->keypad[0x5] = 1;
        break;
      case 'e':
      case 'E':
        chip->keypad[0x6] = 1;
        break;
      case 'r':
      case 'R':
        chip->keypad[0xD] = 1;
        break;
      case 'a':
      case 'A':
        chip->keypad[0x7] = 1;
        break;
      case 's':
      case 'S':
        chip->keypad[0x8] = 1;
        break;
      case 'd':
      case 'D':
        chip->keypad[0x9] = 1;
        break;
      case 'f':
      case 'F':
        chip->keypad[0xE] = 1;
        break;
      case 'z':
      case 'Z':
        chip->keypad[0xA] = 1;
        break;
      case 'x':
      case 'X':
        chip->keypad[0x0] = 1;
        break;
      case 'c':
      case 'C':
        chip->keypad[0xB] = 1;
        break;
      case 'v':
      case 'V':
        chip->keypad[0xF] = 1;
        break;

      // 'Esc' kill switch
      case 27:
      case 3:
        exit(0);
        break;
    }
  }
}

void chip8_decrement_timers(Chip8* chip) {
  if (chip->delay_timer > 0) chip->delay_timer--;
  if (chip->sound_timer > 0) chip->sound_timer--;
}

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
          chip->draw_flag = true;
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

      chip->draw_flag = true;

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
