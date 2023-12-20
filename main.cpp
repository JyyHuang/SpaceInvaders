#include <cstdio>
#include <C:\libraries\glew-2.1.0\include\GL\glew.h>
#include <C:\libraries\glfw-3.3.9.bin.WIN64\include\GLFW\glfw3.h>
#include <iostream>
using namespace std;

void error_callback(int error, const char* description)
{
    cout << stderr <<"Error: " << description << endl;
}

int main(int argc, char* argv[])
{
    glfwSetErrorCallback(error_callback);

    if(!glfwInit())
    {
        return -1;
    }

    // hints for context version
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    // fullscreen mode ?
    GLFWwindow* window = glfwCreateWindow(640, 480, "Space Invaders", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);

    GLenum err = glewInit();
    if (err != GLEW_OK)
    {
        cout << "Error initializing GLEW." << endl;
        glfwTerminate();
        return -1;
    }

    // see which OpenGl version we have
    int glVersion[2] = {-1, 1};
    glGetIntegerv(GL_MAJOR_VERSION, &glVersion[0]);
    glGetIntegerv(GL_MAJOR_VERSION, &glVersion[1]);
    cout << "Using Open Gl: " << glVersion[0] << "." << glVersion[1] << endl;

    // game loop
    
    return 0;
}


