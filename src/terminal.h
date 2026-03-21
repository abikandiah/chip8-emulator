#pragma once

#include <stdint.h>
#include "chip8.h"

int enable_raw_mode(void);
void render_terminal(uint32_t* display);
void handle_terminal_input(uint8_t* keypad);
void terminal_run(Chip8* chip);
