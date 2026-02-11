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

#define OPCODE_MASK     0xF000  // 0b1111000000000000
#define X_MASK          0x0F00  // 0b0000111100000000
#define Y_MASK          0x00F0  // 0b0000000011110000
#define N_MASK          0x000F  // 0b0000000000001111
#define NN_MASK         0x00FF  // 0b0000000011111111
#define NNN_MASK        0x0FFF  // 0b0000111111111111

#define OPCODE_SHIFT   12
#define X_SHIFT        8
#define Y_SHIFT        4

#define OPCODE_0 0x0
#define SUBOPCODE_CLEAR_SCREEN 0x0E0
#define SUBOPCODE_RETURN 0x0EE

#define OPCODE_JMP      0x1
#define OPCODE_CALL         0x2
#define OPCODE_SKIP_EQUAL   0x3
#define OPCODE_SKIP_NO_EQUAL    0x4
#define OPCODE_SET_GP_REG   0x6
#define OPCODE_ADD      0x7

#define OPCODE_8        0x8
#define SUBOPCODE_SET       0x0
#define SUBOPCODE_BINARY_OR 0x1
#define SUBOPCODE_BINARY_AND    0x2
#define SUBOPCODE_LOGICAL_XOR   0x3
#define SUBOPCODE_ADD       0x4
#define SUBOPCODE_SUB       0x5
#define SUBOPCODE_SHIFT_RIGHT   0x6
#define SUBOPCODE_SUB_SWAP  0x7
#define SUBOPCODE_SHIFT_LEFT    0xE

#define OPCODE_SET_I          0xA
#define OPCODE_OFFSET_JUMP        0xB
#define OPCODE_RANDOM             0xC
#define OPCODE_DISPLAY        0xD

#define OPCODE_E                  0xE
#define SUBOPCODE_SKIP_PRESS      0x9E
#define SUBOPCODE_SKIP_NO_PRESS   0xA1

#define OPCODE_F                  0xF
#define SUBOPCODE_GET_DELAY_TIMER 0x07
#define SUBOPCODE_SET_DELAY_TIMER 0x15
#define SUBOPCODE_SET_SOUND_TIMER 0x18
#define SUBOPCODE_ADD_INDEX       0x1E
#define SUBOPCODE_BIN_CODED_DEC   0x33
#define SUBOPCODE_MEM_STORE       0x55
#define SUBOPCODE_MEM_LOAD        0x65

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
            } else if (NNN == SUBOPCODE_RETURN) {
                ctx->sp--;
                ctx->pc = ctx->stack[ctx->sp];
            }
    
            screen_update(ctx);
            screen_draw(ctx, 0, 0);

            break;
        }

        case OPCODE_JMP: {
            ctx->pc = NNN;
            break;
        }

        case OPCODE_CALL: {  
            ctx->stack[ctx->sp] = ctx->pc;  
            ctx->sp++;
            ctx->pc = NNN;
            break;
        }

        case OPCODE_SKIP_EQUAL: {
            if (ctx->gp_regs[X] == NN) {
                ctx->pc += 2;
            }   
            break;      
        }

        case OPCODE_SKIP_NO_EQUAL: {
            if (ctx->gp_regs[X] != NN) {
                    ctx->pc += 2;
            }
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

        case OPCODE_8: {
            switch (N) {
                case SUBOPCODE_SET:
                    ctx->gp_regs[X] = ctx->gp_regs[Y];
                    break;

                case SUBOPCODE_BINARY_OR:
                    ctx->gp_regs[X] |= ctx->gp_regs[Y];
                    break;

                case SUBOPCODE_BINARY_AND:
                    ctx->gp_regs[X] &= ctx->gp_regs[Y];
                    break;

                case SUBOPCODE_LOGICAL_XOR:
                    ctx->gp_regs[X] ^= ctx->gp_regs[Y];
                    break;

                case SUBOPCODE_ADD:
                    ctx->gp_regs[X] += ctx->gp_regs[Y];
                    break;

                case SUBOPCODE_SUB:
                    ctx->gp_regs[X] -= ctx->gp_regs[Y];
                    break;

                case SUBOPCODE_SHIFT_RIGHT:
                    ctx->gp_regs[X] = ctx->gp_regs[Y];
                    ctx->gp_regs[GP_VF] = ctx->gp_regs[X] & 1;
                    ctx->gp_regs[X] >>= 1;
                    break;

                case SUBOPCODE_SUB_SWAP:
                    ctx->gp_regs[X] = ctx->gp_regs[Y] - ctx->gp_regs[X];
                    break;

                case SUBOPCODE_SHIFT_LEFT:
                    ctx->gp_regs[X] = ctx->gp_regs[Y];
                    ctx->gp_regs[GP_VF] = (ctx->gp_regs[X] & 0x80) >> 7;
                    ctx->gp_regs[X] <<= 1;
                    break;

                default:
                    break;
            }
            break;
        }

        case OPCODE_SET_I: {
            ctx->i_reg = NNN;
            break;
        }

        case OPCODE_OFFSET_JUMP: {
            ctx->pc = NNN + ctx->gp_regs[GP_V0];
            break; 
        }

        case OPCODE_RANDOM: {
            ctx->gp_regs[X] = (rand() % NN) & NN;
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
        
        case OPCODE_E: {
            if (NN == SUBOPCODE_SKIP_PRESS) {
                // TODO: Comeback after implementing keyboard
            }
            break;
        }

        case OPCODE_F: {
            if (NN == SUBOPCODE_GET_DELAY_TIMER) {
                ctx->gp_regs[X] = ctx->sp_regs[DELAY_TIMER];
            } else if (NN == SUBOPCODE_SET_DELAY_TIMER) {
                ctx->sp_regs[DELAY_TIMER] = ctx->gp_regs[X];
            } else if (NN == SUBOPCODE_SET_SOUND_TIMER) {
                ctx->sp_regs[SOUND_TIMER] = ctx->gp_regs[X];
            } else if (NN == SUBOPCODE_ADD_INDEX) {
                ctx->i_reg += ctx->gp_regs[X];
            } else if (NN == SUBOPCODE_BIN_CODED_DEC) {
                ctx->memory[ctx->i_reg] = ctx->gp_regs[X] % 1000 / 100;
                ctx->memory[ctx->i_reg + 1] = ctx->gp_regs[X] % 100 / 10;
                ctx->memory[ctx->i_reg + 2] = ctx->gp_regs[X] % 10;
            } else if (NN == SUBOPCODE_MEM_STORE) {
                for (int i = 0; i <= X; i++) {
                    ctx->memory[ctx->i_reg + i] = ctx->gp_regs[i];
                }
            } else if (NN == SUBOPCODE_MEM_LOAD) {
                for (int i = 0; i <= X; i++) {
                    ctx->gp_regs[i] = ctx->memory[ctx->i_reg + i];
                }
            }
            break;
        }
    }
}
