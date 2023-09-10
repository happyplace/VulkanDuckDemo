#include "DuckDemoUtils.h"

#include "Game.h"

SDL_Window* DuckDemoUtils::GetWindow()
{
    return Game::Get()->GetWindow();
}
