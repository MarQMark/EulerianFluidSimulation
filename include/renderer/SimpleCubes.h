//
// Created by ruut on 7/8/26.
//

#ifndef EULERIANFLUIDSIMULATION_SIMPLECUBES_H
#define EULERIANFLUIDSIMULATION_SIMPLECUBES_H

#include "assets/Model.h"
#include "simulation/ScalField3D.h"

class StorageBuffer;

class SimpleCubes {
public:
    SimpleCubes(Mesh* mesh);
    ~SimpleCubes();

    void update(const ScalField3D& field, AABB domain);
    void resizeBuffers(int x, int y, int z);

private:
    Mesh* _mesh;
    Shader* _scShader;

    StorageBuffer* _fieldBuffer{};
    StorageBuffer* _indexBuffer{};
    StorageBuffer* _indexCountBuffer{};
    StorageBuffer* _vertexBuffer{};
    StorageBuffer* _vertexCountBuffer{};

    glm::ivec3 _lastSize = { -1, -1, -1 };
    int _maxVertices = -1;
    int _maxIndices = -1;
};

#endif //EULERIANFLUIDSIMULATION_SIMPLECUBES_H
