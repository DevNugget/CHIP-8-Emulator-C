#include "chip8.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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