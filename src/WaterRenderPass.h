#pragma once

#include <array>
#include <vector>

#include <vulkan/vulkan.h>

#include "VulkanBuffer.h"
#include "RenderObject.h"

struct WaterRenderPass
{
    std::vector<VkFramebuffer> m_vulkanFrameBuffers;
    VkRenderPass m_vulkanRenderPass = VK_NULL_HANDLE;
    VkDescriptorPool m_vulkanDescriptorPool = VK_NULL_HANDLE;
    std::array<VkDescriptorSetLayout, 5> m_vulkanDescriptorSetLayouts = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };
    std::array<VkDescriptorSet, 5> m_vulkanDescriptorSets = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };
    VkPipelineLayout m_vulkanPipelineLayout = VK_NULL_HANDLE;
    VkShaderModule m_vertexShader = VK_NULL_HANDLE;
    VkShaderModule m_fragmentShader = VK_NULL_HANDLE;
    VkPipeline m_vulkanPipeline = VK_NULL_HANDLE;
    VkSampler m_vulkanSampler = VK_NULL_HANDLE;
    VulkanBuffer m_vulkanFrameBuffer;
    VulkanBuffer m_vulkanObjectBuffer;
};

struct WaterRenderPassParams
{
    bool m_wireframe = false;
    uint32_t m_maxRenderObjectCount = 1;
};

bool Init_WaterRenderPass(WaterRenderPass& waterRenderPass, const WaterRenderPassParams& waterRenderPassParams);
void Free_WaterRenderPass(WaterRenderPass& waterRenderPass);

void Resize_WaterRenderPass(WaterRenderPass& waterRenderPass);
void Render_WaterRenderPass(WaterRenderPass& waterRenderPass, VkCommandBuffer commandBuffer, const std::vector<RenderObject>& renderObjects);

void SetWaterImageView_WaterRenderPass(WaterRenderPass& waterRenderPass, VkImageView imageView);
