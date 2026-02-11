#include "chip8.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void load_font_to_mem(chip8_ctx_t* ctx);
void screen_init(chip8_ctx_t* ctx);

void chip8_init(chip8_ctx_t* ctx) {
	memset(ctx->gp_regs, 0, sizeof(ctx->gp_regs));
	memset(ctx->sp_regs, 0, sizeof(ctx->sp_regs));
	memset(ctx->stack, 0, sizeof(ctx->stack));
	ctx->i_reg = 0;
	ctx->pc = PROGRAM_START;
	ctx->sp = 0;
	load_font_to_mem(ctx);
	screen_init(ctx);
}

/*---------------------------------
 |  DEBUG DISPLAY
 ----------------------------------*/
void memdump(uint8_t *memory, uint32_t start, uint32_t end, int columns) {
    printf("Memory Dump [0x%04X - 0x%04X]\n", start, end);
    
    for (uint32_t addr = start; addr <= end; addr += columns) {
        printf("0x%04X:", addr);
        
        for (int c = 0; c < columns; c++) {
            if (addr + c <= end) {
                printf(" %02X", memory[addr + c]);
            }
        }
        printf("\n");
    }
}

void dump_chip8_memory(uint8_t memory[MEMORY_SIZE]) {
    printf("\n[0x000-0x1FF] Interpreter/Reserved:\n");
    memdump(memory, 0x000, 0x1FF, 16);
    
    printf("\n[0x050-0x09F] Font Data:\n");
    memdump(memory, 0x050, 0x09F, 16);
    
    printf("\n[0x200-0xFFF] Program Area (First 1024 bytes):\n");
    memdump(memory, 0x200, 0xFFF, 16);
}

/*---------------------------------
 |  DEFAULT FONT
 ----------------------------------*/
#define FONT_MAP_START 0x050
#define FONT_MAP_END   0x09F

static uint8_t font_map[] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

void load_font_to_mem(chip8_ctx_t* ctx) {
	memcpy(&ctx->memory[FONT_MAP_START], font_map, sizeof(font_map));
}

void load_program_to_mem(chip8_ctx_t* ctx, const char* path) {
	FILE *ptr = fopen(path, "rb");

	while (fread(&ctx->memory[PROGRAM_START], MEMORY_SIZE - PROGRAM_START, 1, ptr)) {

	}

	fclose(ptr);
}

/*---------------------------------
 |  SCREEN
 ----------------------------------*/
void screen_init(chip8_ctx_t* ctx) {
	for (int i = 0; i < SCREEN_SIZE; i++) {
        ctx->pixel_data[i] = BLACK;
    }
    
    Image img = {
        .data = ctx->pixel_data,
        .width = SCREEN_W,
        .height = SCREEN_H,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
        .mipmaps = 1
    };
    
    ctx->screen_texture = LoadTextureFromImage(img);
}

void screen_set(chip8_ctx_t* ctx, int x, int y, Color color) {
    int index = y * SCREEN_W + x;
    
    if (index >= 0 && index < SCREEN_SIZE) {
        ctx->pixel_data[index] = color;
    }
}

void screen_update(chip8_ctx_t* ctx) {
	UpdateTexture(ctx->screen_texture, ctx->pixel_data);
}

void screen_draw(chip8_ctx_t* ctx, int x, int y) {
    DrawTextureEx(ctx->screen_texture, (Vector2){x, y}, 0, SCALE, WHITE);
}


/*---------------------------------
 |  INSTRUCTION FETCH/DECODE/EXEC
 ----------------------------------*/
uint16_t fetch_inst(chip8_ctx_t* ctx) {
	uint16_t inst = (uint16_t)(
		(ctx->memory[ctx->pc] << 8) | 
		(ctx->memory[ctx->pc + 1])
	);
	ctx->pc += 2;
	return inst;
}

#define OPCODE_MASK	0xF000  // 0b1111000000000000
#define X_MASK			0x0F00  // 0b0000111100000000
#define Y_MASK			0x00F0  // 0b0000000011110000
#define N_MASK			0x000F  // 0b0000000000001111
#define NN_MASK			0x00FF  // 0b0000000011111111
#define NNN_MASK		0x0FFF  // 0b0000111111111111

#define OPCODE_SHIFT   12
#define X_SHIFT        8
#define Y_SHIFT        4

#define OPCODE_0 0x0
#define SUBOPCODE_CLEAR_SCREEN 0x0E0

#define OPCODE_JMP 0x1
#define OPCODE_SET_GP_REG 0x6
#define OPCODE_ADD 0x7
#define OPCODE_SET_I 0xA
#define OPCODE_DISPLAY 0xD

void decode_exec_inst(chip8_ctx_t* ctx, uint16_t inst) {
	uint8_t opcode = (inst & OPCODE_MASK) >> OPCODE_SHIFT;
	uint8_t X = (inst & X_MASK) >> X_SHIFT;
	uint8_t Y = (inst & Y_MASK) >> Y_SHIFT;
	uint8_t N = inst & N_MASK;
	uint8_t NN = inst & NN_MASK;
	uint16_t NNN = inst & NNN_MASK;

	switch (opcode) {
		case OPCODE_0: {
			if (NNN == SUBOPCODE_CLEAR_SCREEN) {
				for (int i = 0; i < SCREEN_SIZE; i++) {
						ctx->pixel_data[i] = BLACK;
				}
			}

			screen_update(ctx);
			screen_draw(ctx, 0, 0);

			break;
		}

		case OPCODE_JMP: {
			ctx->pc = NNN;
			break;
		}

		case OPCODE_SET_GP_REG: {
			ctx->gp_regs[X] = NN;
			break;
		}

		case OPCODE_ADD: {
			ctx->gp_regs[X] += NN;
			break;
		}

		case OPCODE_SET_I: {
			ctx->i_reg = NNN;
			break;
		}

		case OPCODE_DISPLAY: {
			// Address of sprite: I
			// width: 8
			// height: N
			// xpos: VX
			// ypos: VY
			uint8_t x_pos = ctx->gp_regs[X] % 64;
			uint8_t y_pos = ctx->gp_regs[Y] % 32;
			ctx->gp_regs[GP_VF] = 0;

			for (int i = 0; i < N; i++) {
				uint8_t sprite_row = ctx->memory[ctx->i_reg+i];

				uint8_t mask = 1 << 7;
				for (int j = 0; j < 8; j++) {
					if (sprite_row & mask) {
						// on
						int screen_x = (x_pos + j) % SCREEN_W;
						int screen_y = (y_pos + i) % SCREEN_H;

						int pixel_index = screen_y * SCREEN_W + screen_x;

						if (ctx->pixel_data[pixel_index].r == 255 &&
							ctx->pixel_data[pixel_index].g == 255 &&
							ctx->pixel_data[pixel_index].b == 255
							) {
							ctx->pixel_data[pixel_index] = BLACK;
							ctx->gp_regs[GP_VF] = 1;
						} else if (ctx->pixel_data[pixel_index].r == 0 &&
								   ctx->pixel_data[pixel_index].g == 0 &&
								   ctx->pixel_data[pixel_index].b == 0
							) {
							ctx->pixel_data[pixel_index] = WHITE;
							ctx->gp_regs[GP_VF] = 1;
						}
					}
					mask >>= 1;
					if (j > SCREEN_W) break;
				}
				if (i > SCREEN_H) break;
			}

			screen_update(ctx);
			screen_draw(ctx, 0, 0);

			break;
		}
	}
}