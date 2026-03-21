#pragma once

#include <stdint.h>

void enable_raw_mode(void);
void render_terminal(uint32_t* display);
void handle_terminal_input(uint8_t* keypad);
