#pragma once

#include <array>

#include <vulkan/vulkan.h>

#include "DuckDemoUtils.h"
#include "VulkanBuffer.h"

struct DUCK_DEMO_ALIGN(16) WaveBuf
{
    float Wc1;
    float Wc2;
    float Wc3;
    float Time;
};

constexpr uint32_t c_waterComputePassTextureCount = 3u;

struct WaterComputePass
{
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    std::array<VkDescriptorSetLayout, 4> descriptorSetLayouts;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    std::array<VkDescriptorSet, 4> descriptorSets;
    std::array<VkImage, c_waterComputePassTextureCount> images = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };
    std::array<VkDeviceMemory, c_waterComputePassTextureCount> deviceMemories = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };
    std::array<VkImageView, c_waterComputePassTextureCount> imageViews = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };
    VkShaderModule shaderModule = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkSemaphore semaphore = VK_NULL_HANDLE;
    uint32_t workGroupDispatchX = 0;
    uint32_t workGroupDispatchY = 0;
    uint32_t prevImageOffset = 0;
    uint32_t currentImageOffset = 1;
    uint32_t nextImageOffset = 2;
    VulkanBuffer waveBufBuffer;
    WaveBuf waveBuf;
};

struct WaterComputePassParams
{
    uint32_t width = 1024;
    float speed = 4.0f;
    float damping = 0.2f;
};

bool Init_WaterComputePass(WaterComputePass& waterComputePass, const WaterComputePassParams& params);
void Free_WaterComputePass(WaterComputePass& waterComputePass);

void Update_WaterComputePass(WaterComputePass& waterComputePass, const double deltaTime);
void Compute_WaterComputePass(WaterComputePass& waterComputePass);
VkImageView GetCurrImageView_WaterComputePass(WaterComputePass& waterComputePass);
