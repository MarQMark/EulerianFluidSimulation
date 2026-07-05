//
// Created by ruut on 4/29/26.
//

#ifndef SPHLASH_MATERIAL_H
#define SPHLASH_MATERIAL_H

#include "Shader.h"
#include <memory>
#include <glm/glm.hpp>
#include "ITexture.h"

enum class FillMode {
    Fill,
    Wireframe
};

class Material {
public:
    Material(const std::string& name, const std::shared_ptr<Shader>& shader);

    void bind() const;

    std::string getName();
    Shader* getShader() const;

    void setFillMode(FillMode mode);
    FillMode getFillMode() const;

    void setCullFaces(bool cull);
    bool getCullFaces() const;

    // -- Shader parameters ---
    void setFloat(const std::string& name, float value);
    void setVec3(const std::string& name, glm::vec3 value);
    void setVec4(const std::string& name, glm::vec4 value);

    void setTexture(const std::string& name, std::shared_ptr<ITexture> tex);

    static Material createWireframe();
private:
    std::string name;
    std::shared_ptr<Shader> shader;

    FillMode fillMode = FillMode::Fill;
    bool cullFaces = true;

    std::unordered_map<std::string, float> floatParams;
    std::unordered_map<std::string, glm::vec3> vec3Params;
    std::unordered_map<std::string, glm::vec4> vec4Params;
    std::unordered_map<std::string, std::shared_ptr<ITexture>> textures;
};

#endif //SPHLASH_MATERIAL_H
