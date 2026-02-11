#include "chip8.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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