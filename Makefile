all:
	gcc -Isrc/include -Lsrc/lib -o chip8 src/*.c -lSDL2
	