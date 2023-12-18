#pragma once

#include <array>

#include "Game.h"
#include "DuckDemoUtils.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "meshloader/Mesh.h"

struct DUCK_DEMO_ALIGN(16) DirectionalLightBuf
{
    glm::vec3 uStrength;
    float padding0;
    glm::vec3 uDirection;
};

struct DUCK_DEMO_ALIGN(16) FrameBuf
{
    glm::mat4 uViewProj;
    glm::vec3 uEyePosW;
    float padding0;
    glm::vec4 uAmbientLight;
    DirectionalLightBuf uDirLight;
};

struct DUCK_DEMO_ALIGN(16) ObjectBuf
{
    glm::mat4 uWorld;
    glm::vec4 uDiffuseAlbedo;
    glm::vec3 uFresnelR0;
    float uRoughness;
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
    std::array<VkDescriptorSetLayout, 2> m_vulkanDescriptorSetLayouts = { VK_NULL_HANDLE, VK_NULL_HANDLE };
    std::array<VkDescriptorSet, 2> m_vulkanDescriptorSets = { VK_NULL_HANDLE, VK_NULL_HANDLE };
    VkPipelineLayout m_vulkanPipelineLayout = VK_NULL_HANDLE;
    VkShaderModule m_vertexShader = VK_NULL_HANDLE;
    VkShaderModule m_fragmentShader = VK_NULL_HANDLE;
    VkPipeline m_vulkanPipeline = VK_NULL_HANDLE;
    
    MeshLoader::Mesh m_mesh;
};
