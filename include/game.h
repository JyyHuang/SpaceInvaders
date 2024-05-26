#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

static const unsigned int MAX_PROJECTILES = 128;

struct Buffer
{
    size_t width, height;
    uint32_t* data;
};

struct Sprite
{
    size_t width, height;
    uint8_t* data;
};

struct Alien
{
    size_t x,y;
    uint8_t type;
};

struct Projectile
{
    size_t x,y;
    int dir;
};

struct Player
{
    size_t x,y;
    size_t lives;
};

struct Game
{
    size_t width, height;
    size_t num_aliens;
    size_t num_projectiles;
    Alien* aliens;
    Player player;
    Projectile projectiles[MAX_PROJECTILES];
};

struct SpriteAnimation
{
    bool loop;
    size_t num_frames;
    size_t frame_duration;
    size_t time;
    Sprite** frames;
};

enum AlienType: uint8_t
{
    ALIEN_DEAD = 0,
    ALIEN_TYPE_A = 1,
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void buffer_clear(Buffer* buffer, uint32_t color);
void buffer_draw_sprite(Buffer* buffer, const Sprite& sprite, size_t x, size_t y, uint32_t color);
void buffer_draw_text(Buffer* buffer, const Sprite& text_sprite_sheet,const char* text, size_t x, size_t y, uint32_t color);
uint32_t rgba_to_uint32(uint8_t r, uint8_t g, uint8_t b, uint8_t a);