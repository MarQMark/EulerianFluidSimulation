#include "compute/ComputeShader.h"

#include <iostream>
#include <GL/glew.h>

ComputeShader::ComputeShader(const std::string& code) {

    const unsigned int compute = glCreateShader(GL_COMPUTE_SHADER);
    const char* source = code.c_str();
    glShaderSource(compute, 1, &source, nullptr);
    glCompileShader(compute);

    GLint status = GL_FALSE;
    glGetShaderiv(compute, GL_COMPILE_STATUS, &status);
    if (status != 1) {
        std::cout << "[SHADER COMPILE] Compute Shader (" << compute <<") Status " << status << std::endl;

        GLsizei logLength = 0;
        GLchar log[1024];
        glGetShaderInfoLog(compute, 1024, &logLength, log);
        std::cout << log << std::endl;

        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR)
            std::cout << "GL Error: " << err << std::endl;
    }

    _id = glCreateProgram();
    glAttachShader(_id, compute);
    glLinkProgram(_id);

    glDeleteShader(compute);
}

ComputeShader::~ComputeShader() {
    glDeleteProgram(_id);
}

void ComputeShader::bind() const {
    glUseProgram(_id);
}

void ComputeShader::dispatch(const int gx, const int gy, const int gz) const {
    glDispatchCompute(gx, gy, gz);
}

void ComputeShader::setInt(const std::string &name, const int v) const {
    glUniform1i(glGetUniformLocation(_id, name.c_str()), v);
}

void ComputeShader::setFloat(const std::string &name, const float v) const {
    glUniform1f(glGetUniformLocation(_id, name.c_str()), v);
}

#include <fstream>
#include <string>

std::string ComputeShader::LoadTextFile(const std::string& path)
{
    std::ifstream file(path);

    if (!file) {
        std::cout << "File (" << path << ") not found" << std::endl;
        return {};
    }

    file.seekg(0, std::ios::end);
    const std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string contents(size, '\0');
    file.read(contents.data(), size);

    return contents;
}