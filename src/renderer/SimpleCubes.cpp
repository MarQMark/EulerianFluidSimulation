//
// Created by ruut on 7/8/26.
//

#include "renderer/SimpleCubes.h"
#include "simulation/data/StorageBuffer.h"
#include "glm/ext/matrix_transform.hpp"
#include "simulation/data/VecField3D.h"
#include "simulation/scene/nodes/FluidNode.h"

SimpleCubes::SimpleCubes(Mesh* mesh) : _mesh(mesh) {
    // --- Shader ---
    _scShader = new Shader({{ "assets/shaders/simple_cubes.comp", Shader::COMPUTE }});

    // -- StorageBuffers ---
    _fieldBuffer = new StorageBuffer(sizeof(float));
    _velBuffer = new StorageBuffer(sizeof(glm::vec4));

    _vertexCountBuffer = new StorageBuffer(sizeof(int));
    _indexCountBuffer = new StorageBuffer(sizeof(int));

    _vertexBuffer = new StorageBuffer(sizeof(Vertex));
    _indexBuffer = new StorageBuffer(sizeof(int));
    _mesh->setBuffers(_vertexBuffer->id(), _indexBuffer->id());
}

SimpleCubes::~SimpleCubes() {
    delete _fieldBuffer;
    delete _vertexCountBuffer;
    delete _indexCountBuffer;
    delete _vertexBuffer;
    delete _indexBuffer;
    delete _scShader;
    delete _velBuffer;
}

void SimpleCubes::update(IGrid3D& grid, AABB domain, RenderingSettings settings, const StorageBuffer* gpuField) {
    if (grid.width() != _lastSize.x || grid.height() != _lastSize.y || grid.depth() != _lastSize.z)
        resizeBuffers(grid.width(), grid.height(), grid.depth());

    _vertexCountBuffer->clear();
    _indexCountBuffer->clear();

    // --- Algorithm ---
    _scShader->bind();
    _scShader->setInt("uMaxVertices", _maxVertices);
    _scShader->setInt("uMaxIndices", _maxIndices);
    _scShader->setInt("uWidth", grid.width());
    _scShader->setInt("uHeight", grid.height());
    _scShader->setInt("uDepth", grid.depth());
    _scShader->setVec3("uDomainSize", domain.size);
    _scShader->setVec3("uDomainCenter", domain.center);
    if (settings.modeColor == GRADIENT) {
        _scShader->setVec3("uGradientColorMin", settings.minColor);
        _scShader->setVec3("uGradientColorMax", settings.maxColor);
    }
    else {
        _scShader->setVec3("uGradientColorMin", settings.color);
        _scShader->setVec3("uGradientColorMax", settings.color);
    }
    _scShader->setFloat("uGradientMin", settings.min);
    _scShader->setFloat("uGradientMax", settings.max);

    if (gpuField) {
        gpuField->bind(0);
    }
    else {
        _fieldBuffer->bind(0);
        _fieldBuffer->upload(grid.rho().data());
    }

    auto* data = (glm::vec4*)malloc(grid.vel().size() * sizeof(glm::vec4));
    for (int i = 0; i < grid.vel().size(); i++) {
        data[i].x = grid.vel().data()[i].x;
        data[i].y = grid.vel().data()[i].y;
        data[i].z = grid.vel().data()[i].z;
    }
    _velBuffer->upload(data);
    free(data);

    _vertexCountBuffer->bind(1);
    _vertexBuffer->bind(2);
    _indexCountBuffer->bind(3);
    _indexBuffer->bind(4);
    _velBuffer->bind(5);

    const int gx = (grid.width() + 7) / 8;
    const int gy = (grid.height() + 7) / 8;
    const int gz = (grid.depth() + 7) / 8;
    _scShader->dispatch(gx, gy, gz);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);

    int vertexCount = 0;
    _vertexCountBuffer->download(&vertexCount);
    _mesh->setVertexCount(vertexCount);

    int indexCount = 0;
    _indexCountBuffer->download(&indexCount);
    _mesh->setIndexCount(indexCount);
}

void SimpleCubes::resizeBuffers(const int x, const int y, const int z) {
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