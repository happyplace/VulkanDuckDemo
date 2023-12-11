#pragma once

#include "meshloader/Mesh.h"

namespace MeshLoader
{
    inline void PrepareMesh(Mesh& outMesh, int vertexCount, int indexCount)
    {
        outMesh.indexCount = indexCount;
        outMesh.vertexCount = vertexCount;

        std::size_t bufferSize = 0;
        bufferSize += outMesh.indexCount * sizeof(IndexType);
        bufferSize += outMesh.vertexCount * sizeof(Vertex);
        outMesh.buffer.reset(new char[bufferSize]);
    }
}
