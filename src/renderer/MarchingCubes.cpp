//
// Created by ruut on 6/28/26.
//

#include "renderer/MarchingCubes.h"

#include "assets/Mesh.h"
#include "assets/Shader.h"
#include "assets/Vertex.h"
#include "simulation/data/StorageBuffer.h"

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
}

void MarchingCubes::updateMesh(const ScalField3D& field, AABB domain) {
    if (field.width() != _lastSize.x || field.height() != _lastSize.y || field.depth() != _lastSize.z)
        resizeBuffers(field.width(), field.height(), field.depth());

    _vertexCountBuffer->clear();
    _indexCountBuffer->clear();

    // --- Algorithm ---
    _mcShader->bind();
    _mcShader->setFloat("uIsoLevel", 0.5f);
    _mcShader->setInt("uMaxVertices", _maxVertices);
    _mcShader->setInt("uMaxIndices", _maxIndices);
    _mcShader->setInt("uWidth", field.width());
    _mcShader->setInt("uHeight", field.height());
    _mcShader->setInt("uDepth", field.depth());
    _mcShader->setVec3("uDomainSize", domain.size);
    _mcShader->setVec3("uDomainCenter", domain.center);

    _edgeToVerticeTable->bind(0);
    _edgeTable->bind(1);
    _triangleTable->bind(2);
    _vertexCountBuffer->bind(3);
    _vertexBuffer->bind(4);
    _indexCountBuffer->bind(5);
    _indexBuffer->bind(6);
    _fieldBuffer->bind(7);
    _fieldBuffer->upload(field.data());

    const int gx = (field.width() + 7) / 8;
    const int gy = (field.height() + 7) / 8;
    const int gz = (field.depth() + 7) / 8;
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

    int estimatedActiveCubes = maxCubes * 0.15f;
    _maxVertices = estimatedActiveCubes * 15 * 1.2f;
    _maxIndices = estimatedActiveCubes * 15 * 1.2f;

    _vertexBuffer->resize(sizeof(Vertex) * _maxVertices);
    _indexBuffer->resize(sizeof(int) * _maxIndices);

    _lastSize = { x, y, z };
}