//
// Created by ruut on 4/29/26.
//

#include "assets/Material.h"
#include <ranges>
#include <utility>

Material::Material(const std::string& name, const std::shared_ptr<Shader>& shader)
    : name(name), shader(shader)
{ }

void Material::bind() const {
    if (!shader)
        return;

    shader->bind();

    for (auto& [param, value] : floatParams)
        shader->setFloat(param, value);
    for (auto& [param, value] : vec3Params)
        shader->setVec3(param, value);
    for (auto& [param, value] : vec4Params)
        shader->setVec4(param, value);

    // Bind textures
    int c = 0;
    for (const auto& value : textures | std::views::values) {
        glActiveTexture(GL_TEXTURE0 + c);
        shader->setInt("param", c);
        value->bind(c);
        c++;
    }
}

std::string Material::getName() {
    return name;
}

Shader* Material::getShader() const {
    return shader.get();
}

void Material::setFillMode(const FillMode mode) {
    fillMode = mode;
}

FillMode Material::getFillMode() const {
    return fillMode;
}

void Material::setCullFaces(bool cull) {
    cullFaces = cull;
}

bool Material::getCullFaces() const {
    return cullFaces;
}

void Material::setFloat(const std::string& name, float value) {
    floatParams[name] = value;
}

void Material::setVec3(const std::string& name, glm::vec3 value) {
    vec3Params[name] = value;
}

void Material::setVec4(const std::string& name, glm::vec4 value) {
    vec4Params[name] = value;
}

void Material::setTexture(const std::string& name, std::shared_ptr<ITexture> tex) {
    textures[name] = std::move(tex);
}

Material Material::createWireframe() {
    Material material("Wireframe", Shader::createDefaultUnshaded());
    material.setFillMode(FillMode::Wireframe);
    material.setCullFaces(false);
    return material;
}
