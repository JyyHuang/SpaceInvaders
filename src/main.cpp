#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cstdint>

using namespace std; 

bool game_running = false;
int player_dir = 0;
bool shoot = false;
static const unsigned int MAX_PROJECTILES = 128;

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void validate_shader(GLuint shader, const char *file = 0){
    static const unsigned int BUFFER_SIZE = 512;
    char buffer[BUFFER_SIZE];
    GLsizei length = 0;

    glGetShaderInfoLog(shader, BUFFER_SIZE, &length, buffer);

    if(length>0){
        cout << "Shader " << shader << "compiler error: " << buffer << endl;
    }
}

bool validate_program(GLuint program){
    static const GLsizei BUFFER_SIZE = 512;
    GLchar buffer[BUFFER_SIZE];
    GLsizei length = 0;

    glGetProgramInfoLog(program, BUFFER_SIZE, &length, buffer);

    if(length>0){
        cout << "Program " << program << "link error: " << buffer;
        return false;
    }

    return true;
}

void press_key(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    switch(key)
    {
        case GLFW_KEY_ESCAPE:
            if (action == GLFW_PRESS)
            {
                game_running = false;
            }
            break;
        case GLFW_KEY_D:
        case GLFW_KEY_RIGHT:
            if (action == GLFW_PRESS)
            {
                player_dir += 1;
            }
            else if(action == GLFW_RELEASE)
            {
                player_dir -= 1;
            }
            break;
        case GLFW_KEY_A:
        case GLFW_KEY_LEFT:
            if (action == GLFW_PRESS){
                player_dir -= 1;
            }
            else if(action == GLFW_RELEASE)
            {
                player_dir += 1;
            }
            break;
        case GLFW_KEY_SPACE:
            if (action == GLFW_RELEASE)
            {
                shoot = true;
            }
            break;
        default:
            break;   
    }
}

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

void create_shaders()
{
    // Create shaders
    static const char* fragment_shader =
        "\n"
        "#version 330\n"
        "\n"
        "uniform sampler2D buffer;\n"
        "noperspective in vec2 TexCoord;\n"
        "\n"
        "out vec3 outColor;\n"
        "\n"
        "void main(void){\n"
        "    outColor = texture(buffer, TexCoord).rgb;\n"
        "}\n";

    static const char* vertex_shader =
        "\n"
        "#version 330\n"
        "\n"
        "noperspective out vec2 TexCoord;\n"
        "\n"
        "void main(void){\n"
        "\n"
        "    TexCoord.x = (gl_VertexID == 2)? 2.0: 0.0;\n"
        "    TexCoord.y = (gl_VertexID == 1)? 2.0: 0.0;\n"
        "    \n"
        "    gl_Position = vec4(2.0 * TexCoord - 1.0, 0.0, 1.0);\n"
        "}\n";

    GLuint shader_id = glCreateProgram();

    {
        //Create vertex shader
        GLuint vshader = glCreateShader(GL_VERTEX_SHADER);

        glShaderSource(vshader, 1, &vertex_shader, 0);
        glCompileShader(vshader);
        validate_shader(vshader, vertex_shader);
        glAttachShader(shader_id, vshader);

        glDeleteShader(vshader);
    }

    {
        //Create fragment shader
        GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);

        glShaderSource(fshader, 1, &fragment_shader, 0);
        glCompileShader(fshader);
        validate_shader(fshader, fragment_shader);
        glAttachShader(shader_id, fshader);

        glDeleteShader(fshader);
    }

    glLinkProgram(shader_id);

    glUseProgram(shader_id);

    GLint location = glGetUniformLocation(shader_id, "buffer");
    glUniform1i(location, 0);
}

bool hit_alien(
    const Sprite& sprite, size_t sprite_x, size_t sprite_y,
    const Sprite& other_sprite, size_t other_sprite_x, size_t other_sprite_y)
{
    if (sprite_x < other_sprite_x + other_sprite.width && other_sprite_x < sprite_x + sprite.width &&
        sprite_y < other_sprite_y + other_sprite.height && other_sprite_y < sprite_y + sprite.height)
        {
            return true;
        }
    return false;
};

int main(int argc, char* argv[])
{
    const size_t buffer_width = 225;
    const size_t buffer_height = 225;

    if (!glfwInit())
    {
        cout << "Failed to initialize GLFW" << endl;
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(buffer_width, buffer_height, "Space Invaders", NULL, NULL);
    if (window == NULL)
    {
        cout << "Failed to open GLFW window" << endl;
        return -1;
    }
    
    glfwSetKeyCallback(window, press_key);

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);


    glfwSwapInterval(1);


    // Graphics buffer
    Buffer buffer;
    buffer.width  = buffer_width;
    buffer.height = buffer_height;
    buffer.data   = new uint32_t[buffer.width * buffer.height];

    buffer_clear(&buffer, 0);

    // Create texture
    GLuint buffer_texture;
    glGenTextures(1, &buffer_texture);
    glBindTexture(GL_TEXTURE_2D, buffer_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, buffer.width, buffer.height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, buffer.data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


    // Create vao for generating fullscreen triangle
    GLuint fullscreen_triangle_vao;
    glGenVertexArrays(1, &fullscreen_triangle_vao);


    create_shaders();

    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);

    glBindVertexArray(fullscreen_triangle_vao);

    Sprite alien_sprites[2];
    // Alien Sprite type A
    alien_sprites[0].width = 11;
    alien_sprites[0].height = 8;
    alien_sprites[0].data = new uint8_t[88]
    {
        0,0,0,0,0,0,0,0,0,0,0, // ...........
        0,0,0,0,0,0,0,0,0,0,0, // ...........
        0,0,1,1,1,1,1,1,1,0,0, // ..@@@@@@@..
        0,1,1,0,1,1,1,0,1,1,0, // .@@.@@@.@@.
        1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
        1,0,1,1,1,1,1,1,1,0,1, // @.@@@@@@@.@
        1,0,1,0,0,0,0,0,1,0,1, // @.@.....@.@
        0,0,0,1,0,0,0,1,0,0,0  // ...@...@...
    };
    // Alien Sprite type A animated
    alien_sprites[1].width = 11;
    alien_sprites[1].height = 8;
    alien_sprites[1].data = new uint8_t[88]
    {
        0,0,0,0,0,0,0,0,0,0,0, // ...........
        1,0,0,0,0,0,0,0,0,0,1, // @.........@
        1,0,1,1,1,1,1,1,1,0,1, // @.@@@@@@@.@
        1,1,1,0,1,1,1,0,1,1,1, // @@@.@@@.@@@
        1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
        0,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@.
        0,0,1,0,0,0,0,0,1,0,0, // ..@.....@..
        0,1,0,0,0,0,0,0,0,1,0  // .@.......@.
    };

    Sprite alien_sprite_dead;
    alien_sprite_dead.width = 13;
    alien_sprite_dead.height = 7;
    alien_sprite_dead.data = new uint8_t[91]
    {
        0,1,0,0,1,0,0,0,1,0,0,1,0, // .@..@...@..@.
        0,0,1,0,0,1,0,1,0,0,1,0,0, // ..@..@.@..@..
        0,0,0,1,0,0,0,0,0,1,0,0,0, // ...@.....@...
        1,1,0,0,0,0,0,0,0,0,0,1,1, // @@.........@@
        0,0,0,1,0,0,0,0,0,1,0,0,0, // ...@.....@...
        0,0,1,0,0,1,0,1,0,0,1,0,0, // ..@..@.@..@..
        0,1,0,0,1,0,0,0,1,0,0,1,0  // .@..@...@..@.
    };

    // Player sprite
    Sprite player_sprite;
    player_sprite.width = 11;
    player_sprite.height = 8;
    player_sprite.data = new uint8_t[88]
    {
        0,0,0,0,0,1,0,0,0,0,0, // .....@.....
        0,0,0,0,1,1,1,0,0,0,0, // ....@@@....
        0,0,0,0,1,1,1,0,0,0,0, // ....@@@....
        0,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@.
        1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
        1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
        1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
        0,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@.
    };

    // Projectile sprite
    Sprite projectile_sprite;
    projectile_sprite.width = 1;
    projectile_sprite.height = 3;
    projectile_sprite.data = new uint8_t[3]
    {
        1, // @
        1, // @
        1, // @
    };

    Sprite text_sprite_sheet;
    text_sprite_sheet.width = 5;
    text_sprite_sheet.height = 7;
    text_sprite_sheet.data = new uint8_t[65 * 35]
    {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,
        0,1,0,1,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,1,0,1,0,0,1,0,1,0,1,1,1,1,1,0,1,0,1,0,1,1,1,1,1,0,1,0,1,0,0,1,0,1,0,
        0,0,1,0,0,0,1,1,1,0,1,0,1,0,0,0,1,1,1,0,0,0,1,0,1,0,1,1,1,0,0,0,1,0,0,
        1,1,0,1,0,1,1,0,1,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,1,0,1,1,0,1,0,1,1,
        0,1,1,0,0,1,0,0,1,0,1,0,0,1,0,0,1,1,0,0,1,0,0,1,0,1,0,0,0,1,0,1,1,1,1,
        0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,1,
        1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,
        0,0,1,0,0,1,0,1,0,1,0,1,1,1,0,0,0,1,0,0,0,1,1,1,0,1,0,1,0,1,0,0,1,0,0,
        0,0,0,0,0,0,0,1,0,0,0,0,1,0,0,1,1,1,1,1,0,0,1,0,0,0,0,1,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,1,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,
        0,0,0,1,0,0,0,0,1,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,1,0,0,0,0,1,0,0,0,

        0,1,1,1,0,1,0,0,0,1,1,0,0,1,1,1,0,1,0,1,1,1,0,0,1,1,0,0,0,1,0,1,1,1,0,
        0,0,1,0,0,0,1,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,1,1,1,0,
        0,1,1,1,0,1,0,0,0,1,0,0,0,0,1,0,0,1,1,0,0,1,0,0,0,1,0,0,0,0,1,1,1,1,1,
        1,1,1,1,1,0,0,0,0,1,0,0,0,1,0,0,0,1,1,0,0,0,0,0,1,1,0,0,0,1,0,1,1,1,0,
        0,0,0,1,0,0,0,1,1,0,0,1,0,1,0,1,0,0,1,0,1,1,1,1,1,0,0,0,1,0,0,0,0,1,0,
        1,1,1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,0,0,1,0,0,0,0,1,1,0,0,0,1,0,1,1,1,0,
        0,1,1,1,0,1,0,0,0,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,1,1,0,0,0,1,0,1,1,1,0,
        1,1,1,1,1,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,
        0,1,1,1,0,1,0,0,0,1,1,0,0,0,1,0,1,1,1,0,1,0,0,0,1,1,0,0,0,1,0,1,1,1,0,
        0,1,1,1,0,1,0,0,0,1,1,0,0,0,1,0,1,1,1,1,0,0,0,0,1,1,0,0,0,1,0,1,1,1,0,

        0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,1,0,0,
        0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,1,
        0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,
        1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,
        0,1,1,1,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,
        0,1,1,1,0,1,0,0,0,1,1,0,1,0,1,1,1,0,1,1,1,0,1,0,0,1,0,0,0,1,0,1,1,1,0,

        0,0,1,0,0,0,1,0,1,0,1,0,0,0,1,1,0,0,0,1,1,1,1,1,1,1,0,0,0,1,1,0,0,0,1,
        1,1,1,1,0,1,0,0,0,1,1,0,0,0,1,1,1,1,1,0,1,0,0,0,1,1,0,0,0,1,1,1,1,1,0,
        0,1,1,1,0,1,0,0,0,1,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,1,0,1,1,1,0,
        1,1,1,1,0,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,1,1,1,0,
        1,1,1,1,1,1,0,0,0,0,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,1,0,0,0,0,1,1,1,1,1,
        1,1,1,1,1,1,0,0,0,0,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,
        0,1,1,1,0,1,0,0,0,1,1,0,0,0,0,1,0,1,1,1,1,0,0,0,1,1,0,0,0,1,0,1,1,1,0,
        1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,1,1,1,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,
        0,1,1,1,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,1,1,1,0,
        0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,1,0,0,0,1,0,1,1,1,0,
        1,0,0,0,1,1,0,0,1,0,1,0,1,0,0,1,1,0,0,0,1,0,1,0,0,1,0,0,1,0,1,0,0,0,1,
        1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,1,1,1,1,
        1,0,0,0,1,1,1,0,1,1,1,0,1,0,1,1,0,1,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,
        1,0,0,0,1,1,0,0,0,1,1,1,0,0,1,1,0,1,0,1,1,0,0,1,1,1,0,0,0,1,1,0,0,0,1,
        0,1,1,1,0,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,0,1,1,1,0,
        1,1,1,1,0,1,0,0,0,1,1,0,0,0,1,1,1,1,1,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,
        0,1,1,1,0,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,1,0,1,1,0,0,1,1,0,1,1,1,1,
        1,1,1,1,0,1,0,0,0,1,1,0,0,0,1,1,1,1,1,0,1,0,1,0,0,1,0,0,1,0,1,0,0,0,1,
        0,1,1,1,0,1,0,0,0,1,1,0,0,0,0,0,1,1,1,0,1,0,0,0,1,0,0,0,0,1,0,1,1,1,0,
        1,1,1,1,1,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,
        1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,0,1,1,1,0,
        1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,0,1,0,1,0,0,0,1,0,0,
        1,0,0,0,1,1,0,0,0,1,1,0,0,0,1,1,0,1,0,1,1,0,1,0,1,1,1,0,1,1,1,0,0,0,1,
        1,0,0,0,1,1,0,0,0,1,0,1,0,1,0,0,0,1,0,0,0,1,0,1,0,1,0,0,0,1,1,0,0,0,1,
        1,0,0,0,1,1,0,0,0,1,0,1,0,1,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,
        1,1,1,1,1,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,1,1,1,1,1,

        0,0,0,1,1,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0,1,1,
        0,1,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,1,0,
        1,1,0,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,1,1,0,0,0,
        0,0,1,0,0,0,1,0,1,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,
        0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    };
    
    // Alien Sprite two-frame animation
    SpriteAnimation* alien_animation = new SpriteAnimation;
    alien_animation->loop = true;
    alien_animation->num_frames = 2;
    alien_animation->frame_duration= 25;
    alien_animation->time = 0;
    alien_animation->frames = new Sprite*[2];
    alien_animation->frames[0] = &alien_sprites[0];
    alien_animation->frames[1] = &alien_sprites[1];


    Game game;
    game.width = buffer_width;
    game.height = buffer_height;
    game.num_projectiles = 0;
    game.num_aliens = 66;
    game.aliens = new Alien[game.num_aliens];
    
    int num_alive = game.num_aliens;

    game.player.x = 113;
    game.player.y = 32;
    game.player.lives = 3;

    // Alien positions
    for(size_t i = 0; i < 6; i++){
        for(size_t j = 0; j < 11; j++)
        {
            Alien& alien = game.aliens[i*11+j];
            alien.type = 1;
            // Iterate over each alien and set positions
            game.aliens[i*11+j].x = 16 * j + 27;
            game.aliens[i*11+j].y = 16 * i + 128;
        }
    }

    uint8_t* death_counters = new uint8_t[game.num_aliens];
    for (size_t i = 0; i < game.num_aliens; i++)
    {
        death_counters[i] = 5;
    }

    game_running = true;

    // Game loop
    while (!glfwWindowShouldClose(window) && game_running)
    {
        buffer_clear(&buffer, rgba_to_uint32(0, 0, 0, 255));

        if (num_alive <= 0){
            buffer_draw_text(
            &buffer,
            text_sprite_sheet, "YOU WIN!",
            92, game.height / 2 + 30,
            rgba_to_uint32(255,255,255,255)
            );
        }

        // Draw aliens
        for (size_t i = 0; i < game.num_aliens; i++)
        {
            if (!death_counters[i]) continue;

            const Alien& alien = game.aliens[i];
            if (alien.type == ALIEN_DEAD)
            {
                buffer_draw_sprite(&buffer, alien_sprite_dead, alien.x, alien.y, rgba_to_uint32(255,255,255,255));
            }
            else
            {
                size_t curr_frame = alien_animation->time / alien_animation->frame_duration;
                const Sprite& sprite = *alien_animation->frames[curr_frame];
                buffer_draw_sprite(&buffer, sprite, alien.x, alien.y, rgba_to_uint32(255,255,255,255));
            }
        }

        // Draw Projectiles
        for (size_t i = 0; i < game.num_projectiles; i++)
        {
            const Projectile& projectile = game.projectiles[i];
            const Sprite& sprite = projectile_sprite;
            buffer_draw_sprite(&buffer, sprite, projectile.x, projectile.y, rgba_to_uint32(255,255,255,255));
        }

        buffer_draw_sprite(&buffer, player_sprite, game.player.x, game.player.y, rgba_to_uint32(255,255,255,255));

        // Check if animation has ended
        // Set time back to 0 if it has, otherwise delete it
        ++alien_animation->time;
        if (alien_animation->time == alien_animation->num_frames * alien_animation->frame_duration)
        {
            if (alien_animation->loop){
                alien_animation-> time = 0;
            }
            else
            {
                delete alien_animation;
                alien_animation = nullptr;
            }
        }


        glTexSubImage2D(
            GL_TEXTURE_2D, 0, 0, 0,
            buffer.width, buffer.height,
            GL_RGBA, GL_UNSIGNED_INT_8_8_8_8,
            buffer.data
        );
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glfwSwapBuffers(window);

        // Update death counter
        for (size_t i = 0; i < game.num_aliens; i++)
        {
            const Alien& alien = game.aliens[i];
            if (alien.type == ALIEN_DEAD && death_counters[i])
            {
                death_counters[i]--;
            } 
        }

        // Update projectile position
        for (size_t i = 0; i < game.num_projectiles; i++)
        {
            game.projectiles[i].y += game.projectiles[i].dir;
            if (game.projectiles[i].y >= game.height || game.projectiles[i].y < projectile_sprite.height)
            {
                game.projectiles[i] = game.projectiles[game.num_projectiles - 1];
                game.num_projectiles--;
                continue;
            }

            for (size_t j = 0; j < game.num_aliens; j++)
            {
                const Alien& alien = game.aliens[j];
                if(alien.type == ALIEN_DEAD) continue;

                size_t curr_frame = alien_animation->time / alien_animation->frame_duration;
                const Sprite& alien_sprite = *alien_animation->frames[curr_frame];

                if (hit_alien(projectile_sprite, game.projectiles[i].x, game.projectiles[i].y, alien_sprite, alien.x, alien.y))
                {
                    num_alive--;
                    game.aliens[j].type = ALIEN_DEAD;
                    game.projectiles[i] = game.projectiles[game.num_projectiles - 1];
                    game.num_projectiles--;
                    continue;
                }
            }
        }

        // game borders and movement
        if(player_dir != 0){
            if(game.player.x + player_sprite.width + player_dir >= game.width)
            {
                game.player.x = game.width - player_sprite.width;
            }
            else if((int)game.player.x + player_dir <= 0)
            {
                game.player.x = 0;
            }
            else
            {
                game.player.x += player_dir;
            }
        }

        if(shoot && game.num_projectiles < MAX_PROJECTILES)
        {
            game.projectiles[game.num_projectiles].x = game.player.x + player_sprite.width / 2;
            game.projectiles[game.num_projectiles].y = game.player.y + player_sprite.height;
            game.projectiles[game.num_projectiles].dir = 2;
            game.num_projectiles++;
        }
        shoot = false;

        glfwPollEvents();

    }

    // Free memory
    glfwDestroyWindow(window);
    glfwTerminate();

    glDeleteVertexArrays(1, &fullscreen_triangle_vao);

    delete[] alien_sprites[0].data;
    delete[] alien_sprites[1].data;
    delete[] alien_sprite_dead.data;
    delete[] player_sprite.data;
    delete[] alien_animation->frames;
    delete[] buffer.data;
    delete[] game.aliens;
    delete[] death_counters;

    delete alien_animation;

    return 0;
}



