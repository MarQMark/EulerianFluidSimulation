//
// Created by ruut on 4/13/26.
//

#ifndef SPHLASH_MODEL_H
#define SPHLASH_MODEL_H

#include <string>
#include "Material.h"
#include "Mesh.h"
#include "simulation/AABB.h"

class Model {
public:
    Model(const std::string& path, bool shaded = true);
    Model(const std::string& name, std::unique_ptr<Mesh> mesh);
    Model(const std::string& name, std::vector<std::unique_ptr<Mesh>> meshes);

    std::string getName() const;
    std::string getPath() const;

    void draw(const glm::mat4& view, const glm::mat4& projection, const glm::mat4& transform) const;

    void updateLocalBoundingBox();
    AABB getBoundingBox() const;
    AABB getWorldBoundingBox(const glm::mat4& transform) const;

    std::vector<Triangle> getWorldSpaceTriangles(const glm::mat4& transform) const;

    void addMesh(std::unique_ptr<Mesh> mesh);
    void setMesh(unsigned int index, std::unique_ptr<Mesh> mesh);
    std::vector<Mesh*> getMeshes() const;

    int addMaterial(const Material& material);
    Material* getMaterial(int index);
    std::vector<Material*> getMaterials();
    void clearMaterials();

    void setInstanceCount(int c);

protected:
    std::string name = "Undefined";
    std::string path = "";

    AABB localBounds;

    std::vector<std::unique_ptr<Mesh>> meshes   = {};
    std::vector<Material> materials             = {};

    int instanceCount = 0;
};

#endif //SPHLASH_MODEL_H
