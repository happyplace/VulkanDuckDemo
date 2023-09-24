#pragma once

#include <vector>

#include <SDL.h>
#include <vulkan/vulkan.h>

#include "GameTimer.h"

class Game
{
public:
    static Game* Get();

    virtual ~Game();

    int Run(int argc, char** argv);

    SDL_Window* GetWindow() const { return m_window; }

    void QuitGame();

protected:
    static Game* ms_instance;

private:
    bool InitWindow();
    bool InitVulkanInstance();
    bool InitVulkanDevice();
    bool InitVulkanSwapChain();
    
    virtual void OnResize() = 0;

    bool m_quit = false;
    GameTimer m_gameTimer;

    SDL_Window* m_window = nullptr;

    VkInstance m_instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_vulkanSurface = VK_NULL_HANDLE;
    uint32_t m_graphicsQueueIndex = 0;
    VkDevice m_vulkanDevice = VK_NULL_HANDLE;
    VkPhysicalDevice m_vulkanPhysicalDevice;
    VkQueue m_vulkanQueue = VK_NULL_HANDLE;
    VkSwapchainKHR m_vulkanSwapchain = VK_NULL_HANDLE;
    std::vector<VkImageView> m_vulkanSwapchainImageViews;
    uint32_t m_vulkanSwapchainWidth = 0;
    uint32_t m_vulkanSwapchainHeight = 0;
    VkFormat m_vulkanSwapchainPixelFormat = VK_FORMAT_UNDEFINED;
#ifdef DUCK_DEMO_VULKAN_DEBUG
    VkDebugReportCallbackEXT m_debugReportCallbackExt = VK_NULL_HANDLE;
#endif // DUCK_DEMO_VULKAN_DEBUG
};
