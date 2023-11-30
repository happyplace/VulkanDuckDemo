#pragma once

#include <cinttypes>

namespace MeshLoader
{
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
    };
}
