#define _POSIX_C_SOURCE 199309L
#include "terminal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

static struct termios original_termios;
static volatile int quit_requested = 0;

static void disable_raw_mode(void) { tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios); }

int enable_raw_mode(void) {
  // Buffer stdout: each pixel is "██" (6 UTF-8 bytes) or "  " (2 bytes),
  // plus 1 newline per row and the "\033[H" cursor reset (3 bytes).
  setvbuf(stdout, NULL, _IOFBF, (64 * 6 + 1) * 32 + 3);

  if (tcgetattr(STDIN_FILENO, &original_termios) == -1) {
    perror("tcgetattr");
    return -1;
  }
  atexit(disable_raw_mode);

  struct termios raw = original_termios;
  raw.c_lflag &= ~(ICANON | ECHO);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 0;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
    perror("tcsetattr");
    return -1;
  }
  return 0;
}

void render_terminal(uint32_t* display) {
  fputs("\033[H", stdout);

  for (int y = 0; y < 32; y++) {
    for (int x = 0; x < 64; x++) {
      fputs(display[y * 64 + x] ? "██" : "  ", stdout);
    }
    fputs("\n", stdout);
  }
  fflush(stdout);
}

void handle_terminal_input(uint8_t* keypad) {
  char c = 0;
  while (read(STDIN_FILENO, &c, 1) == 1) {
    switch (c) {
      case '1':
        keypad[0x1] = 1;
        break;
      case '2':
        keypad[0x2] = 1;
        break;
      case '3':
        keypad[0x3] = 1;
        break;
      case '4':
        keypad[0xC] = 1;
        break;
      case 'q':
      case 'Q':
        keypad[0x4] = 1;
        break;
      case 'w':
      case 'W':
        keypad[0x5] = 1;
        break;
      case 'e':
      case 'E':
        keypad[0x6] = 1;
        break;
      case 'r':
      case 'R':
        keypad[0xD] = 1;
        break;
      case 'a':
      case 'A':
        keypad[0x7] = 1;
        break;
      case 's':
      case 'S':
        keypad[0x8] = 1;
        break;
      case 'd':
      case 'D':
        keypad[0x9] = 1;
        break;
      case 'f':
      case 'F':
        keypad[0xE] = 1;
        break;
      case 'z':
      case 'Z':
        keypad[0xA] = 1;
        break;
      case 'x':
      case 'X':
        keypad[0x0] = 1;
        break;
      case 'c':
      case 'C':
        keypad[0xB] = 1;
        break;
      case 'v':
      case 'V':
        keypad[0xF] = 1;
        break;
      case 3:
        quit_requested = 1;
        return;
      case 27: {
        // Arrow keys and other escape sequences start with ESC (27) followed
        // by more bytes. In non-blocking mode, a bare ESC has nothing after it.
        char seq = 0;
        if (read(STDIN_FILENO, &seq, 1) == 0) {
          quit_requested = 1;  // bare ESC
          return;
        }
        // Consume the rest of the sequence (e.g. '[' + 'A') and ignore it
        if (seq == '[') read(STDIN_FILENO, &seq, 1);
        break;
      }
    }
  }
}

void terminal_run(Chip8* chip) {
  long cycle_delay_ns = 1000000000L / CPU_FREQ;
  int timer_counter = 0;

  while (!quit_requested) {
    struct timespec start, end, sleep_time;
    clock_gettime(CLOCK_MONOTONIC, &start);

    handle_terminal_input(chip->keypad);
    chip8_step(chip);

    // Timers limited to 60Hz
    timer_counter += 60;
    if (timer_counter >= CPU_FREQ) {
      chip8_decrement_timers(chip);
      timer_counter -= CPU_FREQ;
      memset(chip->keypad, 0, KEYPAD_SIZE * sizeof(uint8_t));
    }

    if (chip->draw_flag) {
      render_terminal(chip->display);
      chip->draw_flag = false;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    long elapsed_ns = (end.tv_sec - start.tv_sec) * 1000000000L + (end.tv_nsec - start.tv_nsec);

    if (elapsed_ns < cycle_delay_ns) {
      sleep_time.tv_sec = 0;
      sleep_time.tv_nsec = cycle_delay_ns - elapsed_ns;
      nanosleep(&sleep_time, NULL);
    }
  }
}
