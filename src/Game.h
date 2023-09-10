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

protected:
    static Game* ms_instance;

private:
    bool InitWindow();
    bool InitVulkanInstance();

    bool m_quit = false;
    GameTimer m_gameTimer;

    SDL_Window* m_window = nullptr;

    VkInstance m_instance = VK_NULL_HANDLE;
};
