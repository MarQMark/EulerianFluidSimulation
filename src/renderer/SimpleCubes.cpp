//
// Created by ruut on 7/8/26.
//

#include "renderer/SimpleCubes.h"
#include "simulation/data/StorageBuffer.h"
#include "glm/ext/matrix_transform.hpp"

SimpleCubes::SimpleCubes(Mesh* mesh) : _mesh(mesh) {
    // --- Shader ---
    _scShader = new Shader({{ "assets/shaders/simple_cubes.comp", Shader::COMPUTE }});

    // -- StorageBuffers ---
    _fieldBuffer = new StorageBuffer(sizeof(float));

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
}

void SimpleCubes::update(const ScalField3D& field, AABB domain, const StorageBuffer* gpuField) {
    if (field.width() != _lastSize.x || field.height() != _lastSize.y || field.depth() != _lastSize.z)
        resizeBuffers(field.width(), field.height(), field.depth());

    _vertexCountBuffer->clear();
    _indexCountBuffer->clear();

    // --- Algorithm ---
    _scShader->bind();
    _scShader->setInt("uMaxVertices", _maxVertices);
    _scShader->setInt("uMaxIndices", _maxIndices);
    _scShader->setInt("uWidth", field.width());
    _scShader->setInt("uHeight", field.height());
    _scShader->setInt("uDepth", field.depth());
    _scShader->setVec3("uDomainSize", domain.size);
    _scShader->setVec3("uDomainCenter", domain.center);

    if (gpuField) {
        gpuField->bind(0);
    }
    else {
        _fieldBuffer->bind(0);
        _fieldBuffer->upload(field.data());
    }

    _vertexCountBuffer->bind(1);
    _vertexBuffer->bind(2);
    _indexCountBuffer->bind(3);
    _indexBuffer->bind(4);

    const int gx = (field.width() + 7) / 8;
    const int gy = (field.height() + 7) / 8;
    const int gz = (field.depth() + 7) / 8;
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

    int estimatedActiveCubes = maxCubes * 0.15f;
    _maxVertices = estimatedActiveCubes * 15 * 1.2f;
    _maxIndices = estimatedActiveCubes * 15 * 1.2f;

    _vertexBuffer->resize(sizeof(Vertex) * _maxVertices);
    _indexBuffer->resize(sizeof(int) * _maxIndices);

    _lastSize = { x, y, z };
}