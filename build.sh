gcc src/main.c src/chip8.c src/debug.c src/font.c src/screen.c -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -Iinclude -o chip8
