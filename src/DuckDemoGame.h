#pragma once

#include "Game.h"

class DuckDemoGame : public Game
{
public:
    DuckDemoGame();
    virtual ~DuckDemoGame() override;

private:
    virtual void OnResize() override;
    virtual void OnUpdate(const GameTimer& gameTimer) override;
};
