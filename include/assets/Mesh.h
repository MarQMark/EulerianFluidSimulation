//
// Created by ruut on 4/13/26.
//

#ifndef SPHLASH_MESH_H
#define SPHLASH_MESH_H

#include <memory>
#include <string>
#include "glad/gl.h"
#include "glm/glm.hpp"
#include "Vertex.h"
#include "Triangle.h"

class Mesh {
public:
    Mesh(
        const std::string& name,
        const std::vector<Vertex>& vertices,
        const std::vector<unsigned int>& indices,
        GLenum primitiveMode = GL_TRIANGLES
    );

    Mesh(
        const std::string& name,
        int maxVertices,
        int maxIndices
    );

    ~Mesh();

    void draw() const;
    void drawInstanced(int count) const;

    std::string getName() const;

    void setLineWidth(float lineWidth);

    void updateMeshData(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);

    void setBuffers(unsigned int vertexBuffer, unsigned int indexBuffer);

    void setMaterialIndex(int index);
    int getMaterialIndex() const;

    std::vector<Triangle> getWorldSpaceTriangles(const glm::mat4& model) const;

    void setVertexCount(int vertexCount);
    int getVertexCount() const;
    int getMaxVertexCount() const;

    std::vector<Vertex> getVertices();

    void setIndexCount(int indexCount);
    int getIndexCount() const;
    int getMaxIndexCount() const;

    std::vector<unsigned int> getIndices();

    GLuint getVAO() const;
    GLuint getVBO() const;
    GLuint getEBO() const;

    // --- Factory ---
    static std::unique_ptr<Mesh> createEmpty();
    static std::unique_ptr<Mesh> createBox(const std::string& name);
    static std::unique_ptr<Mesh> createQuad(const std::string& name);
    static std::unique_ptr<Mesh> createWireframeBox(const std::string& name);
    static std::unique_ptr<Mesh> createSphere(const std::string& name, int sectorCount, int stackCount);
private:
    void uploadToGPU();
    void updateGPU() const;

    std::string name;
    int materialIndex = 0;

    std::vector<Vertex>         vertices    = {};
    std::vector<unsigned int>   indices     = {};

    GLuint vao = 0, vbo = 0, ebo = 0;
    GLenum primitiveMode                    = GL_TRIANGLES;
    float lineWidth                         = 2.0f;

    int maxVertices = 0, maxIndices = 0;
    int vertexCount = 0, indexCount = 0;
};

#endif //SPHLASH_MESH_H
