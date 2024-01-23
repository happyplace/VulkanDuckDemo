#pragma once

#include <cinttypes>
#include <memory>

#include "DuckDemoGameDefines.h"
#include "VulkanBuffer.h"
#include "VulkanTexture.h"

struct RenderObject
{
    ObjectBuf objectBuf;
    int32_t objectBufferIndex = -1;

    std::shared_ptr<VulkanTexture> m_texture;

    std::shared_ptr<VulkanBuffer> m_indexBuffer;
    uint32_t m_indexCount = 0;
    std::shared_ptr<VulkanBuffer> m_vertexBuffer;
};
