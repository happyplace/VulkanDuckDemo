#include "VulkanTexture.h"

#include "DuckDemoUtils.h"
#include "Game.h"

void VulkanTexture::Reset()
{
    if (m_image == VK_NULL_HANDLE && m_deviceMemory == VK_NULL_HANDLE)
    {
        return;
    }
    
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
        
    if (m_imageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(vulkanDevice, m_imageView, s_allocator);
        m_imageView = VK_NULL_HANDLE;
    }

    if (m_deviceMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(vulkanDevice, m_deviceMemory, s_allocator);
        m_deviceMemory = VK_NULL_HANDLE;
    }

    if (m_image != VK_NULL_HANDLE)
    {
        vkDestroyImage(vulkanDevice, m_image, s_allocator);
        m_image = VK_NULL_HANDLE;
    }
}
