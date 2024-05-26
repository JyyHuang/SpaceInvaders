#include <iostream>
#include "shaders.h"

using namespace std;

void validate_shader(GLuint shader, const char *file){
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