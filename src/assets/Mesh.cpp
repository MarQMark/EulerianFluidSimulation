//
// Created by ruut on 4/13/26.
//

#include "assets/Mesh.h"
#include "tiny_obj_loader.h"
#include "assets/Primitives.h"
#include "assets/SphereCreator.h"
#include "glm/glm.hpp"

Mesh::Mesh(
    const std::string& name,
    const std::vector<Vertex>& vertices,
    const std::vector<unsigned int>& indices,
    GLenum primitiveMode
)
    : name(name), vertices(vertices), indices(indices), primitiveMode(primitiveMode)
{
    vertexCount = vertices.size();
    maxVertices = vertexCount;

    indexCount = indices.size();
    maxIndices = indexCount;

    uploadToGPU();
}

Mesh::Mesh(const std::string& name, int maxVertices, int maxIndices)
    : name(name), maxVertices(maxVertices), maxIndices(maxIndices)
{
    // VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // VBO
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, maxVertices * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    // EBO
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, maxIndices * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Vertex Attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

Mesh::~Mesh() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
}

void Mesh::draw() const {
    glBindVertexArray(vao);
    glLineWidth(lineWidth);
    glDrawElements(primitiveMode, indexCount, GL_UNSIGNED_INT, nullptr);
}

void Mesh::drawInstanced(int count) const {
    glBindVertexArray(vao);
    glDrawElementsInstanced(primitiveMode, indexCount, GL_UNSIGNED_INT, 0, count);
}

std::string Mesh::getName() const {
    return name;
}

void Mesh::setLineWidth(float lineWidth) {
    this->lineWidth = lineWidth;
}

void Mesh::updateMeshData(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) {
    this->vertices = vertices;
    this->indices = indices;

    vertexCount = vertices.size();
    indexCount = indices.size();

    updateGPU();
}

void Mesh::setBuffers(unsigned int vertexBuffer, unsigned int indexBuffer) {
    // Free old buffers
    if (vbo) glDeleteBuffers(1, &vbo); vbo = 0;
    if (ebo) glDeleteBuffers(1, &ebo); ebo = 0;

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    glEnableVertexAttribArray(2);

    vbo = vertexBuffer;
    ebo = indexBuffer;
}

void Mesh::setMaterialIndex(const int index) {
    materialIndex = index;
}

int Mesh::getMaterialIndex() const {
    return materialIndex;
}

std::vector<Triangle> Mesh::getWorldSpaceTriangles(const glm::mat4& model) const {
    std::vector<Triangle> triangles;

    for (int i = 0; i < indices.size(); i += 3) {
        const glm::vec3 v0 = model * glm::vec4(
            vertices[indices[i    ]].position.x,
            vertices[indices[i    ]].position.y,
            vertices[indices[i    ]].position.z,
            1.0f
        );

        const glm::vec3 v1 = model * glm::vec4(
            vertices[indices[i + 1]].position.x,
            vertices[indices[i + 1]].position.y,
            vertices[indices[i + 1]].position.z,
            1.0f
        );

        const glm::vec3 v2 = model * glm::vec4(
            vertices[indices[i + 2]].position.x,
            vertices[indices[i + 2]].position.y,
            vertices[indices[i + 2]].position.z,
            1.0f
        );

        triangles.push_back({ v0, v1, v2 });
    }

    return triangles;
}

void Mesh::setVertexCount(int vertexCount) {
    this->vertexCount = vertexCount;
}

int Mesh::getVertexCount() const {
    return vertexCount;
}

int Mesh::getMaxVertexCount() const {
    return maxVertices;
}

std::vector<Vertex> Mesh::getVertices() {
    return vertices;
}

void Mesh::setIndexCount(int indexCount) {
    this->indexCount = indexCount;
}

int Mesh::getIndexCount() const {
    return indexCount;
}

int Mesh::getMaxIndexCount() const {
    return maxIndices;
}

std::vector<unsigned int> Mesh::getIndices() {
    return indices;
}

GLuint Mesh::getVAO() const {
    return vao;
}

GLuint Mesh::getVBO() const {
    return vbo;
}

GLuint Mesh::getEBO() const {
    return ebo;
}

std::unique_ptr<Mesh> Mesh::createEmpty() {
    return std::make_unique<Mesh>("Empty", 0, 0);
}

std::unique_ptr<Mesh> Mesh::createCube(const std::string& name) {
    return std::make_unique<Mesh>(
        name,
        Primitives::BoxGeometry::GetVertices(),
        Primitives::BoxGeometry::GetIndices(),
        GL_TRIANGLES
    );
}

std::unique_ptr<Mesh> Mesh::createQuad(const std::string& name) {
    return std::make_unique<Mesh>(
        name,
        Primitives::QuadGeometry::GetVertices(),
        Primitives::QuadGeometry::GetIndices(),
        GL_TRIANGLES
    );
}

std::unique_ptr<Mesh> Mesh::createWireframeCube(const std::string& name) {
    return std::make_unique<Mesh>(
        name,
        Primitives::BoxGeometry::GetVertices(),
        Primitives::BoxGeometry::GetWireframeIndices(),
        GL_LINES
    );
}

std::unique_ptr<Mesh> Mesh::createSphere(const std::string& name, int sectorCount, int stackCount) {
    // Generate mesh data for sphere
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    createSphereData(vertices, indices, sectorCount, stackCount);

    return std::make_unique<Mesh>(
        name,
        vertices,
        indices
    );
}

void Mesh::uploadToGPU() {
    // VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // VBO
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    // EBO
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Vertex Attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void Mesh::updateGPU() const {
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
}
