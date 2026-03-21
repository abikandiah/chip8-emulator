#include "terminal.h"
#include "chip8.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

static struct termios original_termios;

static void disable_raw_mode(void) { tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios); }

void enable_raw_mode(void) {
  // Buffer stdout: each pixel is "██" (6 UTF-8 bytes) or "  " (2 bytes),
  // plus 1 newline per row and the "\033[H" cursor reset (3 bytes).
  setvbuf(stdout, NULL, _IOFBF, (64 * 6 + 1) * 32 + 3);

  tcgetattr(STDIN_FILENO, &original_termios);
  atexit(disable_raw_mode);

  struct termios raw = original_termios;
  raw.c_lflag &= ~(ICANON | ECHO);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 0;

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void render_terminal(uint32_t* display) {
  printf("\033[H");

  for (int y = 0; y < 32; y++) {
    for (int x = 0; x < 64; x++) {
      fputs(display[y * 64 + x] ? "██" : "  ", stdout);
    }
    printf("\n");
  }
  fflush(stdout);
}

void handle_terminal_input(uint8_t* keypad) {
  memset(keypad, 0, KEYPAD_SIZE * sizeof(uint8_t));

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
      case 27:
      case 3:
        exit(0);
        break;
    }
  }
}
