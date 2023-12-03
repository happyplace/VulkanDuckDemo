#pragma once

#include "meshloader/MeshLoaderStd.h"
#include "meshloader/Mesh.h"

namespace MeshLoader
{
    enum class PrimitiveType
    {
        Cube,
    };

    class Loader
    {
    public:
        ML_DLL static bool LoadPrimitive(PrimitiveType primitiveType, Mesh& outMesh);
        ML_DLL static bool LoadModel(const void* buffer, const size_t bufferSize, Mesh& outMesh);
    };
}
