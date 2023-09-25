#include "DuckDemoGame.h"

#include "DuckDemoUtils.h"

DuckDemoGame::DuckDemoGame()
{
    DUCK_DEMO_ASSERT(!ms_instance);
    ms_instance = this;
}

DuckDemoGame::~DuckDemoGame()
{
    DUCK_DEMO_ASSERT(ms_instance == this);
    ms_instance = nullptr;
}

void DuckDemoGame::OnResize()
{

}

void DuckDemoGame::OnUpdate(const GameTimer& gameTimer)
{

}
