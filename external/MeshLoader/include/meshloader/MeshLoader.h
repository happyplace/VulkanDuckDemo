#pragma once

#include <cinttypes>

#include "meshloader/MeshLoaderStd.h"
#include "meshloader/Mesh.h"

namespace MeshLoader
{
    class Loader
    {
    public:
        ML_DLL static bool LoadCubePrimitive(Mesh& outMesh, const float width = 1.0f, const float height = 1.0f, const float depth = 1.0f);
        ML_DLL static bool LoadGridPrimitive(float planeWidth, float planeDepth, uint32_t gridX, uint32_t gridY, Mesh& outMesh);
        ML_DLL static bool LoadModel(const void* buffer, const size_t bufferSize, Mesh& outMesh);
    };
}
