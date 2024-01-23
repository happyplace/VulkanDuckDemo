#include "VulkanBuffer.h"

#include "DuckDemoUtils.h"
#include "Game.h"

void VulkanBuffer::Reset()
{
    if (m_buffer == VK_NULL_HANDLE && m_deviceMemory == VK_NULL_HANDLE)
    {
        return;
    }

    m_deviceSize = 0;

    Game* game = Game::Get();
    if (game == nullptr)
    {
        DUCK_DEMO_ASSERT(game);
        return;
    }
    
    VkDevice vulkanDevice = game->GetVulkanDevice();
    if (vulkanDevice == VK_NULL_HANDLE)
    {
        DUCK_DEMO_ASSERT(vulkanDevice != VK_NULL_HANDLE);
        return;
    }

    if (m_buffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(vulkanDevice, m_buffer, s_allocator);
        m_buffer = VK_NULL_HANDLE;
    }

    if (m_deviceMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(vulkanDevice, m_deviceMemory, s_allocator);
        m_deviceMemory = VK_NULL_HANDLE;
    }
}
