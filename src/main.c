#include <stdio.h>
#include <stdlib.h>

#include "chip8.h"
#include "terminal.h"

int main(int argc, char* argv[]) {
  if (argc < 2) {
    printf("Usage: %s <rom_path>\n", argv[0]);
    return 1;
  }

  Chip8* chip = malloc(sizeof(Chip8));
  if (chip == NULL) {
    fprintf(stderr, "Fatal: Could not allocate memory for Chip-8\n");
    return 1;
  }

  chip8_init(chip);

  if (chip8_load_rom(chip, argv[1]) != 0) {
    free(chip);
    return 1;
  }

  if (enable_raw_mode() != 0) {
    free(chip);
    return 1;
  }

  // blocking
  terminal_run(chip);

  free(chip);
  return 0;
}