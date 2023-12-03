#pragma once

#include "meshloader/Mesh.h"

namespace MeshLoader
{
    class AssimpLoader
    {
    public:
        static bool Load(const void* buffer, const size_t bufferSize, Mesh& outMesh);
    };
}
