#pragma once

#include <vulkan/vulkan.h>

struct VulkanBuffer
{
    ~VulkanBuffer()
    {
        Reset();
    }
    void Reset();

	VkBuffer m_buffer = VK_NULL_HANDLE;
	VkDeviceMemory m_deviceMemory = VK_NULL_HANDLE;
	VkDeviceSize m_deviceSize = 0;
};
