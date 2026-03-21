# Variables
CC = gcc
CFLAGS = -Isrc -Wall -Wextra -std=c11
TARGET = chip8
SRCDIR = src
SRCS = $(SRCDIR)/main.c $(SRCDIR)/chip8.c $(SRCDIR)/terminal.c
OBJS = $(SRCS:.c=.o)

# The default 'all' target
all: $(TARGET)

# Link the object files into the final executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

# Compile .c files into .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up the folder
clean:
	rm -f $(OBJS) $(TARGET)
