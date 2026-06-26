//
// Created by ruut on 3/16/26.
//

#ifndef FLUIDSIMSPH_SHADER_H
#define FLUIDSIMSPH_SHADER_H

#include <memory>
#include <string>
#include <unordered_map>
#include "glm/fwd.hpp"
#include "glad/gl.h"

class Shader {
public:
    enum Type {
        VERTEX,
        FRAGMENT,
        COMPUTE,

        COUNT
    };

    Shader(std::initializer_list<std::pair<std::string, Type>> sources, bool compile = true);
    ~Shader();

    // --- Sources ---
    int addSource(const std::string& path, Type type);
    int addSources(std::initializer_list<std::pair<std::string, Type>> srcs);
    int compile();

    void bind() const;
    void unbind() const;

    void dispatch(int gx, int gy = 1, int gz = 1) const;

    // --- Uniforms ---
    void setFloat(const std::string& name, float value) const;
    void setInt(const std::string& name, int value) const;
    void setBool(const std::string& name, bool value) const;
    void setMat4(const std::string& name, const glm::mat4& value) const;
    void setVec2(const std::string& name, const glm::vec2& value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setVec4(const std::string& name, const glm::vec4& value) const;

    GLuint getId() const;

    // --- Factory ---
    static std::shared_ptr<Shader> createDefaultShaded();
    static std::shared_ptr<Shader> createDefaultUnshaded();
    static std::shared_ptr<Shader> createBox();
    static std::shared_ptr<Shader> createSkybox();

private:
    GLuint shader = 0;
    bool isCompute = false;

    std::unordered_map<Type, std::string> sources;
};

#endif //FLUIDSIMSPH_SHADER_H