//
// Created by ruut on 4/29/26.
//

#ifndef SPHLASH_PRIMITIVES_H
#define SPHLASH_PRIMITIVES_H

#include "Vertex.h"

namespace Primitives {
    struct BoxGeometry {
        // Simple cube data
        //
        //      7 -------- 6
        //     /|         /|
        //    3 -------- 2 |
        //    | |        | |
        //    | 4 -------| 5
        //    |/         |/
        //    0 -------- 1

        static std::vector<Vertex> GetVertices() {
            return {
                { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, 0.0f}, { 0.0f, 0.0f } }, // 0
                { {  0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, 0.0f}, { 0.0f, 0.0f } }, // 1
                { {  0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f, 0.0f}, { 0.0f, 0.0f } }, // 2
                { { -0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f, 0.0f}, { 0.0f, 0.0f } }, // 3
                { { -0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f, 0.0f}, { 0.0f, 0.0f } }, // 4
                { {  0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f, 0.0f}, { 0.0f, 0.0f } }, // 5
                { {  0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f, 0.0f}, { 0.0f, 0.0f } }, // 6
                { { -0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f, 0.0f}, { 0.0f, 0.0f } }  // 7
            };
        }

        static std::vector<unsigned int> GetWireframeIndices() {
            return {
                0, 1, 1, 2, 2, 3, 3, 0,
                4, 5, 5, 6, 6, 7, 7, 4,
                0, 4, 1, 5, 2, 6, 3, 7
            };
        }

        static std::vector<Triangle> GetTriangles() {
            std::vector<Vertex> vertices        = BoxGeometry::GetVertices();
            std::vector<unsigned int> indices   = BoxGeometry::GetIndices();

            std::vector<Triangle> triangles = {};

            for (int i = 0 ; i < indices.size() ; i += 3) {
                triangles.push_back(
                    {
                        vertices[indices[i]].position,
                        vertices[indices[i + 1]].position,
                        vertices[indices[i + 2]].position
                    }
                );
            }

            return triangles;
        }

        static std::vector<unsigned int> GetIndices() {
            return {
                0, 2, 1, 0, 3, 2,
                4, 5, 6, 4, 6, 7,
                1, 6, 5, 1, 2, 6,
                4, 3, 0, 4, 7, 3,
                3, 6, 2, 3, 7, 6,
                4, 1, 5, 4, 0, 1
            };
        }
    };

    struct QuadGeometry {
        static std::vector<Vertex> GetVertices() {
            return {
                { { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f}, { 0.0f, 0.0f } }, // 0
                { {  1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f}, { 1.0f, 0.0f } }, // 1
                { {  1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f}, { 1.0f, 1.0f } }, // 2
                { { -1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f}, { 0.0f, 1.0f } }, // 3
            };
        }

        static std::vector<unsigned int> GetIndices() {
            return {
                0, 1, 2, 0, 2, 3
            };
        }
    };
}

#endif //SPHLASH_PRIMITIVES_H
