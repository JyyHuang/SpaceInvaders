#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

void validate_shader(GLuint shader, const char *file = 0);
bool validate_program(GLuint program);
void create_shaders();