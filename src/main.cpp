#include <SDL.h>

#include "DuckDemoUtils.h"

int main(int argc, char** argv)
{
    SDL_Window* window = nullptr;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0)
    {
        DUCK_DEMO_SHOW_ERROR("SDL Initialization Fail", DuckDemoUtils::format("SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER)\n%s", SDL_GetError()));
        return 1;
    }

    window = SDL_CreateWindow("Vulkan Duck Demo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_SHOWN);
    if (window == nullptr)
    {
        DUCK_DEMO_SHOW_ERROR("SDL Initialization Fail", DuckDemoUtils::format("SDL_CreateWindow\n%s", SDL_GetError()));
        return 1;
    }

    bool quit = false;
    while (!quit)
    {
        SDL_Event sdlEvent;
        while (SDL_PollEvent(&sdlEvent) != 0)
        {
            if (sdlEvent.type == SDL_QUIT)
            {
                quit = true;
                break;
            }
        }

        SDL_UpdateWindowSurface(window);
    }

    if (window)
    {
        SDL_DestroyWindow(window);
    }

    SDL_Quit();
    
    return 0;
}
