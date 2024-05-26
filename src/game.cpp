#include "game.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void buffer_clear(Buffer* buffer, uint32_t color)
{
    for(size_t i = 0; i < buffer->width * buffer->height; i++)
    {
        buffer->data[i] = color;
    }
}

// Goes over sprite and draws pixels at coordinates (x,y)
void buffer_draw_sprite(Buffer* buffer, const Sprite& sprite, size_t x, size_t y, uint32_t color)
{
    for(size_t xi = 0; xi < sprite.width; xi++)
    {
        for(size_t yi = 0; yi < sprite.height; yi++)
        {
            if(sprite.data[yi * sprite.width + xi] &&
               (sprite.height - 1 + y - yi) < buffer->height &&
               (x + xi) < buffer->width)
            {
                buffer->data[(sprite.height - 1 + y - yi) * buffer->width + (x + xi)] = color;
            }
        }
    }
}

void buffer_draw_text(Buffer* buffer, const Sprite& text_sprite_sheet,const char* text, size_t x, size_t y, uint32_t color)
{
    size_t stride = text_sprite_sheet.width * text_sprite_sheet.height;
    Sprite sprite = text_sprite_sheet;
    for(const char* ch = text; *ch != '\0'; ch++)
    {
        char character = *ch - 32;
        if(character < 0 || character >= 65) continue;

        sprite.data = text_sprite_sheet.data + character * stride;
        buffer_draw_sprite(buffer, sprite, x, y, color);
        x += sprite.width + 1;
    }
}

uint32_t rgba_to_uint32(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return (r << 24) | (g << 16) | (b << 8) | a;
}



