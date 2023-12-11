#pragma once

#include <cinttypes>

namespace MeshLoader
{
    struct Vector2
    {
        float x;
        float y;
    };

    struct Vector3
    {
        float x;
        float y;
        float z;
    };

    typedef uint32_t IndexType;

    struct Vertex
    {
        Vector3 position;
        Vector3 normal;
        Vector3 tangent;
        Vector2 texture;
    };
}
