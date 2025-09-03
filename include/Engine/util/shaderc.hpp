#ifndef SHADERC_H
#define SHADERC_H

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include "glad/glad.h"

class Shaderc {

public:
    std::string loadShaderSource(const char* filepath);
    GLuint loadShader(const char* vertexPath, const char* fragmentPath);
};

#endif