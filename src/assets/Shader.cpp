//
// Created by ruut on 3/16/26.
//

#include "assets/Shader.h"
#include <filesystem>

#ifdef HAS_STD_FORMAT
#include <format>
#endif

#include <fstream>
#include "glm/gtc/type_ptr.hpp"
#include "util/Logger.h"

Shader::Shader(std::initializer_list<std::pair<std::string, Type>> sources, bool compile) {
    addSources(sources);

    if (compile)
        this->compile();
}

Shader::~Shader() {
    glDeleteProgram(shader);
}

int Shader::addSource(const std::string& path, Type type) {
    // Validate type
    if (type < 0 || type > Type::COUNT) {
        LOG_ERROR("Invalid shader type");
        return 0;
    }

    // Check if file exists
    if (!std::filesystem::exists(path)) {
        LOG_ERROR("File `" + path + "` does not exist.");
        return 0;
    }

    // Check if regular file
    if (!std::filesystem::is_regular_file(path)) {
        LOG_ERROR("File `" + path + "` is not a regular file.");
        return 0;
    }

    // Read file from disk
    std::ifstream file(path, std::ios::in);
    if (!file.is_open()) {
        LOG_ERROR("Cannot open file `" + path + "`");
        return 0;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    if (file.fail() && !file.eof()) {
        LOG_ERROR("Error while reading file `" + path + "`");
        return 0;
    }

    std::string source = buffer.str();

    if (source.empty()) {
        LOG_ERROR("File `" + path + "` is empty.");
        return 0;
    }

    sources[type] = std::move(source);

    if (type == COMPUTE)
        isCompute = true;

    return 1;
}

int Shader::addSources(std::initializer_list<std::pair<std::string, Type>> srcs) {
    int success = 1;

    for (const auto& source : srcs) {
        if (!addSource(source.first, source.second))
            success = 0;
    }

    return success;
}

int Shader::compile() {
    shader = glCreateProgram();
    std::vector<unsigned int> compiledSources;

    // Compile sources attached to shader
    for (auto& [type, content] : sources) {
        GLenum shaderType;

        switch (type) {
            case Type::VERTEX: shaderType = GL_VERTEX_SHADER; break;
            case Type::FRAGMENT: shaderType = GL_FRAGMENT_SHADER; break;
            case Type::COMPUTE: shaderType = GL_COMPUTE_SHADER; break;
            default: LOG_ERROR("Unknown shader type"); return 0;
        }

        GLuint source = glCreateShader(shaderType);

        // Compile
        int success;
        const char* contentPtr = content.c_str();
        glShaderSource(source, 1, &contentPtr, nullptr);
        glCompileShader(source);

        // Check for compilation errors
        glGetShaderiv(source, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(source, 512, nullptr, infoLog);
            LOG_ERROR("\n\n" + content + "\n\n" + infoLog)

            // Cleanup
            glDeleteProgram(shader);
            shader = -1;
            return 0;
        }

        compiledSources.push_back(source);
        glAttachShader(shader, source);
    }

    // Link shader
    glLinkProgram(shader);

    // Cleanup shaders
    for (const unsigned int shaderID : compiledSources) {
        glDeleteShader(shaderID);
    }

    // Check for linking errors
    int success;
    glGetProgramiv(shader, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shader, 512, nullptr, infoLog);
        LOG_ERROR(infoLog)

        // Cleanup
        glDeleteProgram(shader);
        shader = -1;
        return 0;
    }

    return 1;
}

void Shader::bind() const {
    glUseProgram(shader);
}

void Shader::unbind() const {
    glUseProgram(0);
}

void Shader::dispatch(int gx, int gy, int gz) const {
    if (!isCompute) {
        LOG_WARN("Called dispatch on non-compute shader")
        return;
    }

    glDispatchCompute(gx, gy, gz);
}

void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(shader, name.c_str()), value);
}

void Shader::setInt(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(shader, name.c_str()), value);
}

void Shader::setBool(const std::string& name, bool value) const {
    glUniform1i(glGetUniformLocation(shader, name.c_str()), value);
}

void Shader::setMat4(const std::string& name, const glm::mat4& value) const {
    glUniformMatrix4fv(glGetUniformLocation(shader, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setVec2(const std::string& name, const glm::vec2& value) const {
    glUniform2fv(glGetUniformLocation(shader, name.c_str()), 1, glm::value_ptr(value));
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const {
    glUniform3fv(glGetUniformLocation(shader, name.c_str()), 1, glm::value_ptr(value));
}

void Shader::setVec4(const std::string& name, const glm::vec4& value) const {
    glUniform4fv(glGetUniformLocation(shader, name.c_str()), 1, glm::value_ptr(value));
}

GLuint Shader::getId() const {
    return shader;
}

std::shared_ptr<Shader> Shader::createDefaultShaded() {
    auto shader = std::make_shared<Shader>(
    std::initializer_list<std::pair<std::string, Type>>({
            { "assets/shaders/default.vert", VERTEX },
            { "assets/shaders/shaded.frag", FRAGMENT }
        })
    );

    shader->compile();
    return shader;
}

std::shared_ptr<Shader> Shader::createDefaultUnshaded() {
    auto shader = std::make_shared<Shader>(
    std::initializer_list<std::pair<std::string, Type>>({
            { "assets/shaders/default.vert", VERTEX },
            { "assets/shaders/unshaded.frag", FRAGMENT }
        })
    );

    shader->compile();
    return shader;
}

std::shared_ptr<Shader> Shader::createBox() {
    auto shader = std::make_shared<Shader>(
    std::initializer_list<std::pair<std::string, Type>>({
            { "assets/shaders/box.vert", VERTEX },
            { "assets/shaders/box.frag", FRAGMENT }
        })
    );

    shader->compile();
    return shader;
}

std::shared_ptr<Shader> Shader::createSkybox() {
    auto shader = std::make_shared<Shader>(
    std::initializer_list<std::pair<std::string, Type>>({
            { "assets/shaders/skybox.vert", VERTEX },
            { "assets/shaders/skybox.frag", FRAGMENT }
        })
    );

    shader->compile();
    return shader;
}
