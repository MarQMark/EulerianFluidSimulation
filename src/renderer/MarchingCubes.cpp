//
// Created by ruut on 6/28/26.
//

#include "renderer/MarchingCubes.h"

#include "assets/Mesh.h"
#include "assets/Shader.h"
#include "assets/Vertex.h"
#include "simulation/data/StorageBuffer.h"
#include "simulation/data/VecField3D.h"
#include "simulation/data/ScalField2D.h"
#include "simulation/scene/nodes/FluidNode.h"

MarchingCubes::MarchingCubes(Mesh* mesh) {
    _mesh = mesh;

    // --- Shader ---
    _mcShader = new Shader({{ "assets/shaders/marching_cubes.comp", Shader::COMPUTE }});

    // -- StorageBuffers ---
    _edgeToVerticeTable = new StorageBuffer(sizeof(int) * 12 * 2);
    _edgeToVerticeTable->bind(0);
    _edgeToVerticeTable->upload(edgeToVertices);

    _edgeTable = new StorageBuffer(sizeof(int) * 256);
    _edgeTable->bind(1);
    _edgeTable->upload(edgeTable);

    _triangleTable = new StorageBuffer(sizeof(int) * 256 * 16);
    _triangleTable->bind(2);
    _triangleTable->upload(triangleTable);

    _vertexCountBuffer = new StorageBuffer(sizeof(int));
    _indexCountBuffer = new StorageBuffer(sizeof(int));

    _fieldBuffer = new StorageBuffer(sizeof(float));
    _velBuffer = new StorageBuffer(sizeof(glm::vec4));

    // Assign new marching cubes buffers to mesh. Buffers are resized dynamically
    _vertexBuffer = new StorageBuffer(sizeof(Vertex));
    _indexBuffer = new StorageBuffer(sizeof(int));
    _mesh->setBuffers(_vertexBuffer->id(), _indexBuffer->id());
}

MarchingCubes::~MarchingCubes() {
    delete _mcShader;
    delete _edgeTable;
    delete _edgeToVerticeTable;
    delete _triangleTable;
    delete _velBuffer;
}

void MarchingCubes::updateMesh(IGrid3D& grid, AABB domain, RenderingSettings settings, const StorageBuffer* gpuField) {
    if (grid.width() != _lastSize.x || grid.height() != _lastSize.y || grid.depth() != _lastSize.z)
        resizeBuffers(grid.width(), grid.height(), grid.depth());

    _vertexCountBuffer->clear();
    _indexCountBuffer->clear();

    // --- Algorithm ---
    _mcShader->bind();
    _mcShader->setFloat("uIsoLevel", 0.5f);
    _mcShader->setInt("uMaxVertices", _maxVertices);
    _mcShader->setInt("uMaxIndices", _maxIndices);
    _mcShader->setInt("uWidth", grid.width());
    _mcShader->setInt("uHeight", grid.height());
    _mcShader->setInt("uDepth", grid.depth());
    _mcShader->setVec3("uDomainSize", domain.size);
    _mcShader->setVec3("uDomainCenter", domain.center);
    if (settings.modeColor == GRADIENT) {
        _mcShader->setVec3("uGradientColorMin", settings.minColor);
        _mcShader->setVec3("uGradientColorMax", settings.maxColor);

        auto* data = (glm::vec4*)malloc(grid.vel().size() * sizeof(glm::vec4));
        for (int i = 0; i < grid.vel().size(); i++) {
            data[i].x = grid.vel().data()[i].x;
            data[i].y = grid.vel().data()[i].y;
            data[i].z = grid.vel().data()[i].z;
        }
        _velBuffer->upload(data);
        free(data);
    }
    else {
        _mcShader->setVec3("uGradientColorMin", settings.color);
        _mcShader->setVec3("uGradientColorMax", settings.color);
    }
    _mcShader->setFloat("uGradientMin", settings.min);
    _mcShader->setFloat("uGradientMax", settings.max);

    _edgeToVerticeTable->bind(0);
    _edgeTable->bind(1);
    _triangleTable->bind(2);
    _vertexCountBuffer->bind(3);
    _vertexBuffer->bind(4);
    _indexCountBuffer->bind(5);
    _indexBuffer->bind(6);
    _velBuffer->bind(8);

    if (gpuField) {
        gpuField->bind(7);
    }
    else {
        _fieldBuffer->bind(7);
        _fieldBuffer->upload(grid.rho().data());
    }

    const int gx = (grid.width() + 7) / 8;
    const int gy = (grid.height() + 7) / 8;
    const int gz = (grid.depth() + 7) / 8;
    _mcShader->dispatch(gx, gy, gz);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);

    int vertexCount = 0;
    _vertexCountBuffer->download(&vertexCount);
    _mesh->setVertexCount(vertexCount);

    int indexCount = 0;
    _indexCountBuffer->download(&indexCount);
    _mesh->setIndexCount(indexCount);
}

void MarchingCubes::resizeBuffers(const int x, const int y, const int z) {
    int maxCubes = x * y * z;

    _fieldBuffer->resize(maxCubes * sizeof(float));
    _velBuffer->resize(maxCubes * sizeof(glm::vec4));

    int estimatedActiveCubes = maxCubes * 0.15f;
    _maxVertices = estimatedActiveCubes * 15 * 1.2f;
    _maxIndices = estimatedActiveCubes * 15 * 1.2f;

    _vertexBuffer->resize(sizeof(Vertex) * _maxVertices);
    _indexBuffer->resize(sizeof(int) * _maxIndices);

    _lastSize = { x, y, z };
}