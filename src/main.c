#include <chip8.h>
#include <stdio.h>
#include <time.h>

int main(int argc, char* argv[]) {
  // Seed random
  srand(time(NULL));

  Chip8* chip = malloc(sizeof(Chip8));
  if (chip == NULL) {
    fprintf(stderr, "Fatal: Could not allocate memory for Chip-8\n");
    return 1;
  }

  chip8_init(chip);
  // chip_8_load_rom();
  chip8_step();

  free(chip);

  return 0;
}