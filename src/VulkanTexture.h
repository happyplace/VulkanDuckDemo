#pragma once

#include <vulkan/vulkan.h>

struct VulkanTexture
{
    ~VulkanTexture()
    {
        Reset();
    }
    void Reset();

    VkImage m_image = VK_NULL_HANDLE;
    VkDeviceMemory m_deviceMemory = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
};
