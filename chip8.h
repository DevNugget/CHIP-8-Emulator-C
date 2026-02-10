#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>

#define MEMORY_SIZE 4096
#define STACK_SIZE 16
#define GENERAL_PURPOSE_REG_COUNT 16
#define TIMER_REG_COUNT 2

#define GP_V0 0x0
#define GP_V1 0x1
#define GP_V2 0x2
#define GP_V3 0x3
#define GP_V4 0x4
#define GP_V5 0x5
#define GP_V6 0x6
#define GP_V7 0x7
#define GP_V8 0x8
#define GP_V9 0x9
#define GP_VA 0xA
#define GP_VB 0xB
#define GP_VC 0xC
#define GP_VD 0xD
#define GP_VE 0xE

// Should not be used by programs
// Some instructions use this as a flag
#define GP_VF 0xF 

#define SOUND_TIMER 0x0
#define DELAY_TIMER 0x1

#define SPRITE_SIZE 15

typedef struct chip8_ctx {
	uint8_t memory[MEMORY_SIZE];
	uint8_t gp_regs[GENERAL_PURPOSE_REG_COUNT];
	uint8_t sp_regs[TIMER_REG_COUNT];

	// Used to store memory addresses
	// Only right-most 12 bits are used
	uint16_t i_reg;

	uint16_t pc;
	uint8_t sp;
	uint16_t stack[STACK_SIZE];
} chip8_ctx_t;

typedef struct sprite {
	uint8_t data[SPRITE_SIZE];
} sprite_t;

#endif