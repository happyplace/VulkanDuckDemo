#pragma once

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

    bool m_quit = false;
    GameTimer m_gameTimer;

    SDL_Window* m_window = nullptr;

    VkInstance m_instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_vulkanSurface = VK_NULL_HANDLE;
    uint32_t m_graphicsQueueIndex = 0;
    VkDevice m_vulkanDevice = VK_NULL_HANDLE;
    VkQueue m_vulkanQueue = VK_NULL_HANDLE;
#ifdef DUCK_DEMO_VULKAN_DEBUG
    VkDebugReportCallbackEXT m_debugReportCallbackExt = VK_NULL_HANDLE;
#endif // DUCK_DEMO_VULKAN_DEBUG
};
