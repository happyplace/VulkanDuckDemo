#include "meshloader/MeshLoader.h" 

#include <cstring>

#include "MeshUtils.h"

#ifdef ML_ENABLE_ASSIMP
#include "AssimpLoader.h"
#endif // ML_ENABLE_ASSIMP

using namespace MeshLoader;

bool Loader::LoadCubePrimitive(Mesh& outMesh, const float width /*= 1.0f*/, const float height /*= 1.0f*/, const float depth /*= 1.0f*/)
{
    const float halfWidth = width * 0.5f;
    const float halfHeight = height * 0.5f;
    const float halfDepth = depth * 0.5f;

    const Vertex cubeVertexData[] =
    {
        {
            { -halfWidth, -halfHeight, -halfDepth }, 
            { 0.0f, 0.0f, -1.0f }, 
            { 1.0f, 0.0f, 0.0f }, 
            { 0.0f, 1.0f }
        },
        {
            { -halfWidth, +halfHeight, -halfDepth }, 
            { 0.0f, 0.0f, -1.0f }, 
            { 1.0f, 0.0f, 0.0f }, 
            { 0.0f, 0.0f }
        },
        {
            { +halfWidth, +halfHeight, -halfDepth }, 
            { 0.0f, 0.0f, -1.0f }, 
            { 1.0f, 0.0f, 0.0f }, 
            { 1.0f, 0.0f }
        },
        {
            { +halfWidth, -halfHeight, -halfDepth },
            { 0.0f, 0.0f, -1.0f }, 
            { 1.0f, 0.0f, 0.0f }, 
            { 1.0f, 1.0f }
        },
        {
            { -halfWidth, -halfHeight, +halfDepth },
            { 0.0f, 0.0f, 1.0f },
            { -1.0f, 0.0f, 0.0f }, 
            { 1.0f, 1.0f }
        },
        {
            { +halfWidth, -halfHeight, +halfDepth }, 
            { 0.0f, 0.0f, 1.0f }, 
            { -1.0f, 0.0f, 0.0f }, 
            { 0.0f, 1.0f }
        },
        {
            { +halfWidth, +halfHeight, +halfDepth }, 
            { 0.0f, 0.0f, 1.0f }, 
            { -1.0f, 0.0f, 0.0f }, 
            { 0.0f, 0.0f }
        },
        {
            { -halfWidth, +halfHeight, +halfDepth }, 
            { 0.0f, 0.0f, 1.0f }, 
            { -1.0f, 0.0f, 0.0f }, 
            { 1.0f, 0.0f }
        },
        {
            { -halfWidth, +halfHeight, -halfDepth }, 
            { 0.0f, 1.0f, 0.0f },
            { 1.0f, 0.0f, 0.0f },
            { 0.0f, 1.0f }
        },
        {
            { -halfWidth, +halfHeight, +halfDepth }, 
            { 0.0f, 1.0f, 0.0f }, 
            { 1.0f, 0.0f, 0.0f }, 
            { 0.0f, 0.0f }
        },
        {
            { +halfWidth, +halfHeight, +halfDepth }, 
            { 0.0f, 1.0f, 0.0f }, 
            { 1.0f, 0.0f, 0.0f }, 
            { 1.0f, 0.0f }
        },
        {
            { +halfWidth, +halfHeight, -halfDepth }, 
            { 0.0f, 1.0f, 0.0f }, 
            { 1.0f, 0.0f, 0.0f }, 
            { 1.0f, 1.0f }
        },
        {
            { -halfWidth, -halfHeight, -halfDepth },
            { 0.0f, -1.0f, 0.0f }, 
            { -1.0f, 0.0f, 0.0f }, 
            { 1.0f, 1.0f }
        },
        {
            { +halfWidth, -halfHeight, -halfDepth }, 
            { 0.0f, -1.0f, 0.0f }, 
            { -1.0f, 0.0f, 0.0f }, 
            { 0.0f, 1.0f }
        },
        {
            { +halfWidth, -halfHeight, +halfDepth }, 
            { 0.0f, -1.0f, 0.0f }, 
            { -1.0f, 0.0f, 0.0f }, 
            { 0.0f, 0.0f }
        },
        {
            { -halfWidth, -halfHeight, +halfDepth }, 
            { 0.0f, -1.0f, 0.0f }, 
            { -1.0f, 0.0f, 0.0f }, 
            { 1.0f, 0.0f }
        },
        {
            { -halfWidth, -halfHeight, +halfDepth }, 
            { -1.0f, 0.0f, 0.0f }, 
            { 0.0f, 0.0f, -1.0f }, 
            { 0.0f, 1.0f }
        },
        {
            { -halfWidth, +halfHeight, +halfDepth }, 
            { -1.0f, 0.0f, 0.0f }, 
            { 0.0f, 0.0f, -1.0f }, 
            { 0.0f, 0.0f }
        },
        {
            { -halfWidth, +halfHeight, -halfDepth }, 
            { -1.0f, 0.0f, 0.0f }, 
            { 0.0f, 0.0f, -1.0f }, 
            { 1.0f, 0.0f }
        },
        {
            { -halfWidth, -halfHeight, -halfDepth }, 
            { -1.0f, 0.0f, 0.0f }, 
            { 0.0f, 0.0f, -1.0f }, 
            { 1.0f, 1.0f }
        },
        {
            { +halfWidth, -halfHeight, -halfDepth }, 
            { 1.0f, 0.0f, 0.0f }, 
            { 0.0f, 0.0f, 1.0f }, 
            { 0.0f, 1.0f }
        },
        {
            { +halfWidth, +halfHeight, -halfDepth }, 
            { 1.0f, 0.0f, 0.0f }, 
            { 0.0f, 0.0f, 1.0f }, 
            { 0.0f, 0.0f }
        },
        {
            { +halfWidth, +halfHeight, +halfDepth }, 
            { 1.0f, 0.0f, 0.0f }, 
            { 0.0f, 0.0f, 1.0f }, 
            { 1.0f, 0.0f }
        },
        {
            { +halfWidth, -halfHeight, +halfDepth }, 
            { 1.0f, 0.0f, 0.0f }, 
            { 0.0f, 0.0f, 1.0f }, 
            { 1.0f, 1.0f }
        }
    };

    constexpr IndexType cubeIndexData[] =
    {
        0, 1, 2,
        0, 2, 3,
        4, 5, 6,
        4, 6, 7,
        8, 9, 10,
        8, 10, 11,
        12, 13, 14,
        12, 14, 15,
        16, 17, 18,
        16, 18, 19,
        20, 21, 22,
        20, 22, 23
    };

    constexpr std::size_t vertexSize = sizeof(Vertex);
    constexpr std::size_t cubeVertexDataSize = sizeof(cubeVertexData);
    constexpr int cubeVertexDataCount = static_cast<int>(cubeVertexDataSize / vertexSize);

    constexpr std::size_t indexSize = sizeof(IndexType);
    constexpr std::size_t cubeIndexDataSize = sizeof(cubeIndexData);
    constexpr int cubeIndexDataCount = static_cast<int>(cubeIndexDataSize / indexSize);

    PrepareMesh(outMesh, cubeVertexDataCount, cubeIndexDataCount);  

    if (outMesh.buffer.get() == nullptr)
    {
        return false;
    }

    memcpy(outMesh.GetVertex(), cubeVertexData, cubeVertexDataSize);
    memcpy(outMesh.GetIndex(), cubeIndexData, cubeIndexDataSize);

    return true;
}

bool Loader::LoadModel(const void* buffer, const size_t bufferSize, Mesh& outMesh)
{
#ifdef ML_ENABLE_ASSIMP
    return AssimpLoader::Load(buffer, bufferSize, outMesh);
#else
    return false;
#endif // ML_ENABLE_ASSIMP
}
