#include "AssimpLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "MeshUtils.h"

using namespace MeshLoader;
using namespace Assimp;

unsigned CalculateMeshRefInNodeAndChildrenCount(aiNode* node)
{
    unsigned count = node->mNumMeshes;
    for (unsigned i = 0; i < node->mNumChildren; ++i)
    {
        count += CalculateMeshRefInNodeAndChildrenCount(node->mChildren[i]);
    }
    return count;
}

bool AssimpLoader::Load(const void* buffer, const size_t bufferSize, Mesh& outMesh)
{
    Importer importer;

    const aiScene* scene = importer.ReadFileFromMemory(buffer, bufferSize, aiProcessPreset_TargetRealtime_Quality | aiProcess_ConvertToLeftHanded);
    if (scene == nullptr)
    {
        //std::string error = importer.GetErrorString();
        return false;
    }

    // only supporting one mesh per model right now so one total mesh in the scene, and only one of the nodes should reference that mesh
    if (scene->mNumMeshes > 1)
    {
        return false;
    }

    const unsigned meshRefCount = CalculateMeshRefInNodeAndChildrenCount(scene->mRootNode);
    if (meshRefCount > 1)
    {
        return false;
    }

    aiMesh* assimpMesh = scene->mMeshes[0];
    PrepareMesh(outMesh, assimpMesh->mNumVertices, assimpMesh->mNumFaces * assimpMesh->mFaces[0].mNumIndices);
    
    for (unsigned i = 0; i < assimpMesh->mNumVertices; ++i)
    {
        outMesh.GetVertex()[i].position.x = assimpMesh->mVertices[i].x;
        outMesh.GetVertex()[i].position.y = assimpMesh->mVertices[i].y;
        outMesh.GetVertex()[i].position.z = assimpMesh->mVertices[i].z;
    }

    if (assimpMesh->mNormals)
    {
        for (unsigned i = 0; i < assimpMesh->mNumVertices; ++i)
        {
            outMesh.GetVertex()[i].normal.x = assimpMesh->mNormals[i].x;
            outMesh.GetVertex()[i].normal.y = assimpMesh->mNormals[i].y;
            outMesh.GetVertex()[i].normal.z = assimpMesh->mNormals[i].z;
        }
    }

    if (assimpMesh->mTangents)
    {
        for (unsigned i = 0; i < assimpMesh->mNumVertices; ++i)
        {
            outMesh.GetVertex()[i].tangent.x = assimpMesh->mTangents[i].x;
            outMesh.GetVertex()[i].tangent.y = assimpMesh->mTangents[i].y;
            outMesh.GetVertex()[i].tangent.z = assimpMesh->mTangents[i].z;
        }
    }

    if (assimpMesh->HasTextureCoords(0))
    {
        for (unsigned i = 0; i < assimpMesh->mNumVertices; ++i)
        {
            outMesh.GetVertex()[i].texture.x = assimpMesh->mTextureCoords[0][i].x;
            outMesh.GetVertex()[i].texture.y = assimpMesh->mTextureCoords[0][i].y;
        }
    }

    for (unsigned i = 0; i < assimpMesh->mNumFaces; ++i)
    {
        const unsigned baseIndex = i * 3;
        for (unsigned k = 0; k < assimpMesh->mFaces[i].mNumIndices; ++k)
        {
            outMesh.GetIndex()[baseIndex + k] = assimpMesh->mFaces[i].mIndices[k];
        }
    }

    return true;
}
