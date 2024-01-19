#pragma once

#include <array>

#include "Game.h"
#include "DuckDemoUtils.h"

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

#include "meshloader/Mesh.h"

struct DUCK_DEMO_ALIGN(16) DirectionalLightBuf
{
    glm::vec3 uStrength;
    float padding0;
    glm::vec3 uDirection;
};

struct DUCK_DEMO_ALIGN(16) SpotLightBuf
{
    glm::vec3 uStrength;
    float uFalloffStart;
    glm::vec3 uDirection;
    float uFalloffEnd;
    glm::vec3 uPosition;
    float uSpotPower;
};

struct DUCK_DEMO_ALIGN(16) PointLightBuf
{
    glm::vec3 uStrength;
    float uFalloffStart;
    glm::vec3 uPosition;
    float uFalloffEnd;
};

struct DUCK_DEMO_ALIGN(16) FrameBuf
{
    glm::mat4 uViewProj;
    glm::vec3 uEyePosW;
    float padding0;
    glm::vec4 uAmbientLight;
    DirectionalLightBuf uDirLight;
    SpotLightBuf uSpotLight;
    PointLightBuf uPointLights[2];
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
    virtual void OnImGui() override;

    void UpdateFrameBuffer();

    std::vector<VkFramebuffer> m_vulkanFrameBuffers;

    VulkanBuffer m_vulkanFrameBuffer;
    VulkanBuffer m_vulkanObjectBuffer;

    VulkanBuffer m_vulkanDuckIndexBuffer;
    VulkanBuffer m_vulkanDuckVertexBuffer;

    VulkanBuffer m_vulkanFloorIndexBuffer;
    VulkanBuffer m_vulkanFloorVertexBuffer;

    VkRenderPass m_vulkanRenderPass;
    VkDescriptorPool m_vulkanDescriptorPool = VK_NULL_HANDLE;
    std::array<VkDescriptorSetLayout, 3> m_vulkanDescriptorSetLayouts = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };
    std::array<VkDescriptorSet, 3> m_vulkanDescriptorSets = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };
    VkPipelineLayout m_vulkanPipelineLayout = VK_NULL_HANDLE;
    VkShaderModule m_vertexShader = VK_NULL_HANDLE;
    VkShaderModule m_fragmentShader = VK_NULL_HANDLE;
    VkPipeline m_vulkanPipeline = VK_NULL_HANDLE;
    VkSampler m_vulkanSampler = VK_NULL_HANDLE;
    
    MeshLoader::Mesh m_duckMesh;
    MeshLoader::Mesh m_floorMesh;

    glm::quat m_cameraRotation;
    glm::vec3 m_cameraPosition;
    float m_cameraRotationX;
    float m_cameraRotationY;

    VulkanTexture m_vulkanDuckDiffuseTexture;
};
