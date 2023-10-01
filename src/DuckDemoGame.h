#pragma once

#include "Game.h"

class DuckDemoGame : public Game
{
public:
    DuckDemoGame();
    virtual ~DuckDemoGame() override;

private:
    bool InitRenderPass();

    virtual VkRenderPass GetRenderPass() override { return m_vulkanRenderPass; }

    virtual bool OnInit() override;
    virtual void OnResize() override;
    virtual void OnUpdate(const GameTimer& gameTimer) override;
    virtual void OnRender() override;

    VkRenderPass m_vulkanRenderPass;
};
