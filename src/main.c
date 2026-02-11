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
    const int instructions_per_frame = 10;

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        if (chip8.sp_regs[DELAY_TIMER] > 0) chip8.sp_regs[DELAY_TIMER]--;
        if (chip8.sp_regs[SOUND_TIMER] > 0) chip8.sp_regs[SOUND_TIMER]--;

        for (int i = 0; i < instructions_per_frame; i++) {
            uint16_t inst = fetch_inst(&chip8);
            decode_exec_inst(&chip8, inst);
        }

        BeginDrawing();
            screen_draw(&chip8, 0, 0);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}