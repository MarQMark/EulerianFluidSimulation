//
// Created by ruut on 7/8/26.
//

#ifndef EULERIANFLUIDSIMULATION_SIMPLECUBES_H
#define EULERIANFLUIDSIMULATION_SIMPLECUBES_H

#include "assets/Model.h"
#include "simulation/data/IGrid.h"
#include "simulation/data/ScalField3D.h"
#include "simulation/scene/nodes/SimulationDomainNode.h"

struct RenderingSettings;
class StorageBuffer;

class SimpleCubes {
public:
    SimpleCubes(Mesh* mesh);
    ~SimpleCubes();

    void update(IGrid3D& grid, AABB domain, RenderingSettings settings, const StorageBuffer* gpuField);
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
    StorageBuffer * _velBuffer;
};

#endif //EULERIANFLUIDSIMULATION_SIMPLECUBES_H
