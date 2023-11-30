#pragma once

#include <memory>

#include "meshloader/Vertex.h"

namespace MeshLoader
{
    struct Mesh
    {
        std::unique_ptr<char> buffer = nullptr;
        
        Vertex* GetVertex() { return reinterpret_cast<Vertex*>(&buffer.get()[0]); }
        int vertexCount = 0;

        IndexType* GetIndex() { return reinterpret_cast<IndexType*>(&buffer.get()[sizeof(Vertex)*vertexCount]); }
        int indexCount = 0;
    };
}
