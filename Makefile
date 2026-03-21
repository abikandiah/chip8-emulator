# Variables
CC = gcc
CFLAGS = -I. -Wall -Wextra -std=c11
TARGET = chip8
SRC = main.c chip8.c
OBJ = $(SRC:.c=.o)

# The default 'all' target
all: $(TARGET)

# Link the object files into the final executable
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

# Compile .c files into .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up the folder
clean:
	rm -f $(OBJ) $(TARGET)
