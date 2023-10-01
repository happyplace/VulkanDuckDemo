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

    virtual VkRenderPass GetRenderPass() = 0;

    virtual bool OnInit() = 0;
    virtual void OnResize() = 0;
    virtual void OnUpdate(const GameTimer& gameTimer) = 0;
    virtual void OnRender() = 0;

    VkDevice m_vulkanDevice = VK_NULL_HANDLE;
    VkFormat m_vulkanSwapchainPixelFormat = VK_FORMAT_UNDEFINED;
    VkClearValue m_vulkanClearValue;
    VkCommandBuffer m_vulkanPrimaryCommandBuffer = VK_NULL_HANDLE;
    uint32_t m_vulkanSwapchainWidth = 0;
    uint32_t m_vulkanSwapchainHeight = 0;
    std::vector<VkFramebuffer> m_vulkanSwapchainFrameBuffers;
    uint32_t m_currentSwapchainImageIndex = 0;

private:
    bool InitWindow();
    bool InitVulkanInstance();
    bool InitVulkanDevice();
    bool InitVulkanSwapChain();
    bool InitVulkanGameResources();
    bool InitFrameBuffers();

    void DestroyFrameBuffers();

    void Update();
    void Resize();
    void BeginRender();
    void EndRender();

    bool m_quit = false;
    GameTimer m_gameTimer;

    SDL_Window* m_window = nullptr;
    VkInstance m_instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_vulkanSurface = VK_NULL_HANDLE;
    uint32_t m_vulkanGraphicsQueueIndex = 0;
    VkPhysicalDevice m_vulkanPhysicalDevice;
    VkQueue m_vulkanQueue = VK_NULL_HANDLE;
    VkSwapchainKHR m_vulkanSwapchain = VK_NULL_HANDLE;
    std::vector<VkImageView> m_vulkanSwapchainImageViews;
    VkSemaphore m_vulkanAquireSwapchain = VK_NULL_HANDLE;
    VkSemaphore m_vulkanReleaseSwapchain = VK_NULL_HANDLE;
    VkCommandPool m_vulkanPrimaryCommandPool;
    VkFence m_vulkanSubmitFence = VK_NULL_HANDLE;

#ifdef DUCK_DEMO_VULKAN_DEBUG
    VkDebugReportCallbackEXT m_debugReportCallbackExt = VK_NULL_HANDLE;
#endif // DUCK_DEMO_VULKAN_DEBUG
};
