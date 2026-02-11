#include "raylib.h"
#include "chip8.h"

#include <stdio.h>

int main(int argc, char** argv) {
    InitWindow(SCREEN_W * SCALE, SCREEN_H * SCALE, "CHIP 8 Emulator");
    chip8_ctx_t chip8;
    chip8_init(&chip8);

    if (argc > 1) {
        load_program_to_mem(&chip8, argv[1]);
    } else {
        printf("ERROR: No program file provided!\nUSAGE: chip8 <program path>\n");
        return -1;
    }

    dump_chip8_memory(chip8.memory);

    while (!WindowShouldClose())
    {
        BeginDrawing();
            screen_draw(&chip8, 0,0);
            uint16_t inst = fetch_inst(&chip8);
            decode_exec_inst(&chip8, inst);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}