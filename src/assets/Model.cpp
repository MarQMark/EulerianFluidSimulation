//
// Created by ruut on 4/13/26.
//

#define GLM_ENABLE_EXPERIMENTAL

#include "assets/Model.h"
#include <filesystem>
#include <iostream>
#include "tiny_obj_loader.h"
#include "glm/gtc/quaternion.hpp"
#include "util/Logger.h"

Model::Model(const std::string& path, bool shaded) {
    name = std::filesystem::path(path).stem().string();
    this->path = path;

    // Load model from disk
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string baseDir = std::filesystem::path(path).parent_path().string();

    bool success = tinyobj::LoadObj(
        &attrib,
        &shapes,
        &materials,
        nullptr,
        nullptr,
        path.c_str(),
        baseDir.c_str()
    );

    if (!success) {
        LOG_ERROR("Failed to load model on path '" + path + "'")
    }

    // Parse object
    struct VertexKey {
        int v, vt, vn;

        bool operator<(const VertexKey& other) const {
            return std::tie(v, vt, vn) < std::tie(other.v, other.vt, other.vn);
        }
    };

    for (const auto& shape : shapes) {
        // Maps global VertexKey to local (per mesh) vertices index
        std::map<VertexKey, unsigned int> vertexToIndex;

        // Mesh data
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        for (const auto& idx : shape.mesh.indices) {
            VertexKey key {
                idx.vertex_index,
                idx.texcoord_index,
                idx.normal_index
            };

            // Check if vertex with given indices already exists
            if (!vertexToIndex.contains(key)) {
                Vertex vertex;

                // Position
                float vx = attrib.vertices[3 * idx.vertex_index + 0];
                float vy = attrib.vertices[3 * idx.vertex_index + 1];
                float vz = attrib.vertices[3 * idx.vertex_index + 2];
                vertex.position = { vx, vy, vz };

                // Normal
                float nx = attrib.normals[3 * idx.normal_index + 0];
                float ny = attrib.normals[3 * idx.normal_index + 1];
                float nz = attrib.normals[3 * idx.normal_index + 2];
                vertex.normal = { nx, ny, nz };

                // Texture
                float tx = attrib.texcoords[2 * idx.texcoord_index + 0];
                float ty = attrib.texcoords[2 * idx.texcoord_index + 1];
                vertex.texCoord = { tx, ty };

                vertexToIndex[key] = vertices.size();
                vertices.push_back(vertex);
            }

            indices.push_back(vertexToIndex[key]);
        }

        auto mesh = std::make_unique<Mesh>(shape.name, vertices, indices);

        // Material
        if (shape.mesh.material_ids[0] != -1) {
            auto mat = materials[shape.mesh.material_ids[0]];

            // Create default material
            auto shader = shaded ? Shader::createDefaultShaded() : Shader::createDefaultUnshaded();
            Material material(mat.name, shader);
            material.setVec3("color", {mat.diffuse[0], mat.diffuse[1], mat.diffuse[2] });

            mesh->setMaterialIndex(this->materials.size());
            this->materials.push_back(material);
        }

        meshes.push_back(std::move(mesh));
    }

    // Add default model material if no other material was loaded
    Material material("Default", Shader::createDefaultShaded());
    material.setVec3("color", { 1.0f, 1.0f, 1.0f });
    this->materials.push_back(material);

    updateLocalBoundingBox();
}

Model::Model(const std::string& name, std::unique_ptr<Mesh> mesh) {
    this->name = name;

    if (mesh)
        meshes.push_back(std::move(mesh));

    updateLocalBoundingBox();
}

Model::Model(const std::string& name, std::vector<std::unique_ptr<Mesh>> meshes) : meshes(std::move(meshes)) {
    this->name = name;

    updateLocalBoundingBox();
}

std::string Model::getName() const {
    return name;
}

std::string Model::getPath() const {
    return path;
}

void Model::draw(const glm::mat4& view, const glm::mat4& projection, const glm::mat4& transform) const {
    if (materials.empty()) {
        std::cerr << "Model " << name << " does not have any materials attached." << std::endl;
        return;
    }

    glm::vec3 cameraPos = glm::vec3(glm::inverse(view)[3]);

    glDepthMask(GL_TRUE);

    for (const auto& mesh : meshes) {
        // Set material
        int matIndex = mesh->getMaterialIndex();
        if (matIndex != -1) {
            // Bind material
            auto material = materials[matIndex];

            material.bind();

            material.getCullFaces() ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
            glPolygonMode(GL_FRONT_AND_BACK, material.getFillMode() == FillMode::Fill ? GL_FILL : GL_LINE);

            material.getShader()->setMat4("view", view);
            material.getShader()->setVec3("viewPos", cameraPos);
            material.getShader()->setMat4("projection", projection);
            material.getShader()->setMat4("model", transform);

            // Draw mesh
            if (instanceCount > 0) {
                mesh->drawInstanced(instanceCount);
            } else {
                mesh->draw();
            }
        } else {
            std::cerr << "Mesh " << mesh->getName() << " does not have a material" << std::endl;
        }
    }
}

void Model::updateLocalBoundingBox() {
    glm::vec3 min(FLT_MAX);
    glm::vec3 max(-FLT_MAX);

    for (const auto& mesh : meshes) {
        for (const auto& v : mesh->getVertices()) {
            min = glm::min(min, { v.position.x , v.position.y , v.position.z });
            max = glm::max(max, { v.position.x , v.position.y , v.position.z });
        }
    }

    localBounds.size = max - min;
    localBounds.center = min + localBounds.size * 0.5f;
}

AABB Model::getBoundingBox() const {
    return localBounds;
}

AABB Model::getWorldBoundingBox(const glm::mat4& transform) const {
    glm::vec3 min(FLT_MAX);
    glm::vec3 max(-FLT_MAX);

    for (const auto& mesh : meshes) {
        for (const auto& v : mesh->getVertices()) {
            glm::vec3 world = glm::vec3(transform * glm::vec4(v.position.x , v.position.y , v.position.z, 1.0f));
            min = glm::min(min, world);
            max = glm::max(max, world);
        }
    }

    glm::vec3 size = max - min;
    glm::vec3 center = min + size * 0.5f;

    return { center, size };
}

std::vector<Triangle> Model::getWorldSpaceTriangles(const glm::mat4& transform) const {
    std::vector<Triangle> triangles;

    for (auto& mesh : meshes) {
        auto meshTriangles = mesh->getWorldSpaceTriangles(transform);
        triangles.insert(triangles.end(), meshTriangles.begin(), meshTriangles.end());
    }

    return triangles;
}

void Model::addMesh(std::unique_ptr<Mesh> mesh) {
    meshes.push_back(std::move(mesh));

    updateLocalBoundingBox();
}

void Model::setMesh(unsigned int index, std::unique_ptr<Mesh> mesh) {
    if (index < 0 || index >= meshes.size())
        return;

    meshes[index] = std::move(mesh);
}

std::vector<Mesh*> Model::getMeshes() const {
    std::vector<Mesh*> rawMeshes;
    rawMeshes.reserve(meshes.size());

    for (auto& m : meshes)
        rawMeshes.push_back(m.get());

    return rawMeshes;
}

int Model::addMaterial(const Material& material) {
    int matIndex = materials.size();
    materials.push_back(material);
    return matIndex;
}

Material* Model::getMaterial(const int index) {
    if (index < 0 || index >= materials.size()) {
        std::cerr << "Material index out of bounds" << std::endl;
    }

    return &materials[index];
}

std::vector<Material*> Model::getMaterials() {
    std::vector<Material*> mats;

    for (int i = 0 ; i < materials.size(); i++)
        mats.push_back(&materials[i]);

    return mats;
}

void Model::clearMaterials() {
    materials.clear();
}

void Model::setInstanceCount(int c) {
    instanceCount = c;
}
