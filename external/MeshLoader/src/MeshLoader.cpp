#include "meshloader/MeshLoader.h" 

#include <cstring>

using namespace MeshLoader;

void PrepareMesh(Mesh& outMesh, int vertexCount, int indexCount)
{
    outMesh.indexCount = indexCount;
    outMesh.vertexCount = vertexCount;

    std::size_t bufferSize = 0;
    bufferSize += outMesh.indexCount * sizeof(IndexType);
    bufferSize += outMesh.vertexCount * sizeof(Vertex);
    outMesh.buffer.reset(new char[bufferSize]);
}

bool CubePrimitive(Mesh& outMesh)
{
    constexpr Vertex cubeVertexData[] =
    {
        {
            { -1.0f, -1.0f, -1.0f },
        },
        {
            { 1.0f, -1.0f, -1.0f }
        },
        {
            { -1.0f, 1.0f, -1.0f }
        },
        {
            { 1.0f, 1.0f, -1.0f }
        },
        {
            { -1.0f, -1.0f, 1.0f }
        },
        {
            { 1.0f, -1.0f, 1.0f }
        },
        {
            { -1.0f, 1.0f, 1.0f }
        },
        {
            { 1.0f, 1.0f, 1.0f }
        },
    };

    constexpr IndexType cubeIndexData[] =
    {
        0, 2, 3,
        0, 3, 1,
        4, 5, 7,
        4, 7, 6,
        1, 3, 7,
        1, 7, 5,
        0, 4, 6,
        0, 6, 2,
        2, 6, 7,
        2, 7, 3,
        0, 1, 5,
        0, 5, 4,
    };

    constexpr std::size_t vertexSize = sizeof(Vertex);
    constexpr std::size_t cubeVertexDataSize = sizeof(cubeVertexData);
    constexpr int cubeVertexDataCount = static_cast<int>(cubeVertexDataSize / vertexSize);

    constexpr std::size_t indexSize = sizeof(IndexType);
    constexpr std::size_t cubeIndexDataSize = sizeof(cubeIndexData);
    constexpr int cubeIndexDataCount = static_cast<int>(cubeIndexDataSize / indexSize);

    PrepareMesh(outMesh, cubeVertexDataCount, cubeIndexDataCount);  

    memcpy(outMesh.GetVertex(), cubeVertexData, cubeVertexDataSize);
    memcpy(outMesh.GetIndex(), cubeIndexData, cubeIndexDataSize);

    return true;
}

bool Loader::LoadPrimitive(PrimitiveType primitiveType, Mesh& outMesh)
{
    switch (primitiveType)
    {
        case PrimitiveType::Cube:
            return CubePrimitive(outMesh);
        default:
            return false;
    }
}
