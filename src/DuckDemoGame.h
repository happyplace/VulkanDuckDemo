#pragma once

#include <array>

#include "Game.h"
#include "DuckDemoUtils.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

struct DUCK_DEMO_ALIGN(16) FrameBuf
{
    glm::mat4 uViewProj;
};

struct DUCK_DEMO_ALIGN(16) ObjectBuf
{
    glm::mat4 uWorld;
};

struct DUCK_DEMO_ALIGN(16) Vertex
{
    glm::vec3 aPosition;
};

class DuckDemoGame : public Game
{
public:
    DuckDemoGame();
    virtual ~DuckDemoGame() override;

private:
    bool InitFrameBuffers();
    void DestroyFrameBuffers();

    virtual bool OnInit() override;
    virtual void OnResize() override;
    virtual void OnUpdate(const GameTimer& gameTimer) override;
    virtual void OnRender() override;

    std::vector<VkFramebuffer> m_vulkanFrameBuffers;

    VulkanBuffer m_vulkanFrameBuffer;
    VulkanBuffer m_vulkanObjectBuffer;

    VulkanBuffer m_vulkanIndexBuffer;
    VulkanBuffer m_vulkanVertexBuffer;

    VkRenderPass m_vulkanRenderPass;
    VkDescriptorPool m_vulkanDescriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_vulkanDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_vulkanDescriptorSet = VK_NULL_HANDLE;
    VkPipelineLayout m_vulkanPipelineLayout = VK_NULL_HANDLE;
    VkShaderModule m_vertexShader = VK_NULL_HANDLE;
    VkShaderModule m_fragmentShader = VK_NULL_HANDLE;
    VkPipeline m_vulkanPipeline = VK_NULL_HANDLE;
};
