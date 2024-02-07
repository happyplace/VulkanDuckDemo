#include "WaterComputePass.h"

#include <limits>
#include <vector>

#include "Game.h"
#include "DuckDemoUtils.h"

constexpr uint32_t c_numWorkGroupShaderX = 16;
constexpr uint32_t c_numWorkGroupShaderY = 16;

bool Init_WaterComputePass(WaterComputePass& waterComputePass, const WaterComputePassParams& params)
{
    VkResult result = VK_SUCCESS;

    std::array<VkDescriptorPoolSize, 2> descriptorPoolSize;
    descriptorPoolSize[0].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    descriptorPoolSize[0].descriptorCount = 3;
    descriptorPoolSize[1].type = VK_DESCRIPTOR_TYPE_SAMPLER;
    descriptorPoolSize[1].descriptorCount = 1;

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo;
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.pNext = nullptr;
    descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    descriptorPoolCreateInfo.maxSets = 3;
    descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSize.size());
    descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSize.data();

    result = vkCreateDescriptorPool(Game::Get()->GetVulkanDevice(), &descriptorPoolCreateInfo, s_allocator, &waterComputePass.descriptorPool);
    DUCK_DEMO_VULKAN_ASSERT(result);
    if (result != VK_SUCCESS)
    {
        return false;
    }

    for (std::size_t i = 0; i < waterComputePass.descriptorSetLayouts.size(); ++i)
    {
        VkDescriptorSetLayoutBinding descriptorSetLayoutBinding;
        descriptorSetLayoutBinding.binding = 0;
        descriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptorSetLayoutBinding.descriptorCount = 1;
        descriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        descriptorSetLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
        descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutCreateInfo.pNext = nullptr;
        descriptorSetLayoutCreateInfo.flags = 0;
        descriptorSetLayoutCreateInfo.bindingCount = 1;
        descriptorSetLayoutCreateInfo.pBindings = &descriptorSetLayoutBinding;

        result = vkCreateDescriptorSetLayout(Game::Get()->GetVulkanDevice(), &descriptorSetLayoutCreateInfo, s_allocator, &waterComputePass.descriptorSetLayouts[i]);
        DUCK_DEMO_VULKAN_ASSERT(result);
        if (result != VK_SUCCESS)
        {
            return false;
        }
    }   

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.pNext = nullptr;
    pipelineLayoutCreateInfo.flags = 0;
    pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(waterComputePass.descriptorSetLayouts.size());
    pipelineLayoutCreateInfo.pSetLayouts = waterComputePass.descriptorSetLayouts.data();
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    result = vkCreatePipelineLayout(Game::Get()->GetVulkanDevice(), &pipelineLayoutCreateInfo, s_allocator, &waterComputePass.pipelineLayout);
    DUCK_DEMO_VULKAN_ASSERT(result);
    if (result != VK_SUCCESS)
    {
        return false;
    }

    for (std::size_t i = 0; i < waterComputePass.descriptorSets.size(); ++i)
    {
        VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
        descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocateInfo.pNext = nullptr;
        descriptorSetAllocateInfo.descriptorPool = waterComputePass.descriptorPool;
        descriptorSetAllocateInfo.descriptorSetCount = 1;
        descriptorSetAllocateInfo.pSetLayouts = &waterComputePass.descriptorSetLayouts[i];

        result = vkAllocateDescriptorSets(Game::Get()->GetVulkanDevice(), &descriptorSetAllocateInfo, &waterComputePass.descriptorSets[i]);
        DUCK_DEMO_VULKAN_ASSERT(result);
        if (result != VK_SUCCESS)
        {
            return false;
        }
    }

    for (uint32_t i = 0; i < c_waterComputePassTextureCount; ++i)
    {    
        VkImageCreateInfo imageCreateInfo;
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.pNext = nullptr;
        imageCreateInfo.flags = 0;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format = VK_FORMAT_R32_SFLOAT;
        imageCreateInfo.extent.width = params.width;
        imageCreateInfo.extent.height = params.width;
        imageCreateInfo.extent.depth = 1;
        imageCreateInfo.mipLevels = 1;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.queueFamilyIndexCount = 0;
        imageCreateInfo.pQueueFamilyIndices = nullptr;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        DUCK_DEMO_VULKAN_ASSERT(vkCreateImage(Game::Get()->GetVulkanDevice(), &imageCreateInfo, s_allocator, &waterComputePass.images[i]));

        VkMemoryRequirements memoryRequirements;
        vkGetImageMemoryRequirements(Game::Get()->GetVulkanDevice(), waterComputePass.images[i], &memoryRequirements);

        VkMemoryAllocateInfo memoryAllocateInfo;
        memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocateInfo.pNext = nullptr;
        memoryAllocateInfo.allocationSize = memoryRequirements.size;
        memoryAllocateInfo.memoryTypeIndex = Game::Get()->FindMemoryByFlagAndType(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryRequirements.memoryTypeBits);
    
        DUCK_DEMO_VULKAN_ASSERT(vkAllocateMemory(Game::Get()->GetVulkanDevice(), &memoryAllocateInfo, s_allocator, &waterComputePass.deviceMemories[i]));
        DUCK_DEMO_VULKAN_ASSERT(vkBindImageMemory(Game::Get()->GetVulkanDevice(), waterComputePass.images[i], waterComputePass.deviceMemories[i], 0));

        VkImageViewCreateInfo imageViewCreateInfo;
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.pNext = nullptr;
        imageViewCreateInfo.flags = 0;
        imageViewCreateInfo.image = waterComputePass.images[i];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = VK_FORMAT_R32_SFLOAT;
        imageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;
        DUCK_DEMO_VULKAN_ASSERT(vkCreateImageView(Game::Get()->GetVulkanDevice(), &imageViewCreateInfo, s_allocator, &waterComputePass.imageViews[i]));
    }

    std::array<VkDescriptorImageInfo, c_waterComputePassTextureCount> descriptorImageInfos;
    for (uint32_t i = 0; i < c_waterComputePassTextureCount; ++i)
    {
        descriptorImageInfos[i].sampler = nullptr;
        descriptorImageInfos[i].imageView = waterComputePass.imageViews[i];
        descriptorImageInfos[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    }
    
    for (uint32_t i = 0; i < c_waterComputePassTextureCount; ++i)
    {
        VkWriteDescriptorSet writeDescriptorSet;
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.pNext = nullptr;
        writeDescriptorSet.dstSet = waterComputePass.descriptorSets[i];
        writeDescriptorSet.dstBinding = 0;
        writeDescriptorSet.dstArrayElement = 0;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writeDescriptorSet.pBufferInfo = nullptr;
        writeDescriptorSet.pImageInfo = &descriptorImageInfos[i];
        writeDescriptorSet.pTexelBufferView = nullptr;

        vkUpdateDescriptorSets(Game::Get()->GetVulkanDevice(), 1, &writeDescriptorSet, 0, nullptr);
    }

    shaderc_compile_options_t compileOptions = shaderc_compile_options_initialize();
    if (compileOptions == nullptr)
    {
        DUCK_DEMO_ASSERT(false);
        return false;
    }

    std::array<std::pair<std::string, std::string>, 2> defines;
    defines[0].first = "NUM_GROUPS_X";
    defines[0].second = std::to_string(c_numWorkGroupShaderX);
    defines[1].first = "NUM_GROUPS_Y";
    defines[1].second = std::to_string(c_numWorkGroupShaderY);

    for (std::pair<std::string, std::string>& definePair : defines)
    {
        shaderc_compile_options_add_macro_definition(compileOptions, 
            definePair.first.c_str(), static_cast<size_t>(definePair.first.size()), 
            definePair.second.c_str(), static_cast<size_t>(definePair.second.size()));
    }

    result = Game::Get()->CompileShaderFromDisk("data/shader_src/water.comp", shaderc_glsl_compute_shader, &waterComputePass.shaderModule, compileOptions);
    DUCK_DEMO_VULKAN_ASSERT(result);
    if (result != VK_SUCCESS)
    {
        return false;
    }

    VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo;
    pipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineShaderStageCreateInfo.pNext = nullptr;
    pipelineShaderStageCreateInfo.flags = 0;
    pipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineShaderStageCreateInfo.module = waterComputePass.shaderModule;
    pipelineShaderStageCreateInfo.pName = "main";
    pipelineShaderStageCreateInfo.pSpecializationInfo = nullptr;

    VkComputePipelineCreateInfo computePipelineCreateInfo;
    computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineCreateInfo.pNext = nullptr;
    computePipelineCreateInfo.flags = 0;
    computePipelineCreateInfo.stage = pipelineShaderStageCreateInfo;
    computePipelineCreateInfo.layout = waterComputePass.pipelineLayout;
    computePipelineCreateInfo.basePipelineHandle = nullptr;
    computePipelineCreateInfo.basePipelineIndex = 0;

    result = vkCreateComputePipelines(Game::Get()->GetVulkanDevice(), nullptr, 1, &computePipelineCreateInfo, s_allocator, &waterComputePass.pipeline);
    DUCK_DEMO_VULKAN_ASSERT(result);
    if (result != VK_SUCCESS)
    {
        return false;
    }

    VkCommandPoolCreateInfo commandPoolCreateInfo;
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.pNext = nullptr;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCreateInfo.queueFamilyIndex = Game::Get()->GetVulkanComputeQueueIndex();

    result = vkCreateCommandPool(Game::Get()->GetVulkanDevice(), &commandPoolCreateInfo, s_allocator, &waterComputePass.commandPool);
    DUCK_DEMO_VULKAN_ASSERT(result);
    if (result != VK_SUCCESS)
    {
        return false;
    }

    VkCommandBufferAllocateInfo commandBufferAllocateInfo;
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.pNext = nullptr;
    commandBufferAllocateInfo.commandPool = waterComputePass.commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;

    result = vkAllocateCommandBuffers(Game::Get()->GetVulkanDevice(), &commandBufferAllocateInfo, &waterComputePass.commandBuffer);
    DUCK_DEMO_VULKAN_ASSERT(result);
    if (result != VK_SUCCESS)
    {
        return false;
    }

    VkSemaphoreCreateInfo semaphoreCreateInfo;
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = nullptr;
    semaphoreCreateInfo.flags = 0;
    result = vkCreateSemaphore(Game::Get()->GetVulkanDevice(), &semaphoreCreateInfo, s_allocator, &waterComputePass.semaphore);
    DUCK_DEMO_VULKAN_ASSERT(result);
    if (result != VK_SUCCESS)
    {
        return false;
    }

    VkQueue computeQueue = VK_NULL_HANDLE;
    vkGetDeviceQueue(Game::Get()->GetVulkanDevice(), Game::Get()->GetVulkanComputeQueueIndex(), 0, &computeQueue);

    VkSubmitInfo submitInfo;
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = VK_NULL_HANDLE;
    submitInfo.pWaitDstStageMask = VK_NULL_HANDLE;
    submitInfo.commandBufferCount = 0;
    submitInfo.pCommandBuffers = VK_NULL_HANDLE;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &waterComputePass.semaphore;
    result = vkQueueSubmit(computeQueue, 1, &submitInfo, VK_NULL_HANDLE);
    DUCK_DEMO_VULKAN_ASSERT(result);
    if (result != VK_SUCCESS)
    {
        return false;
    }

    result = vkQueueWaitIdle(computeQueue);
    DUCK_DEMO_VULKAN_ASSERT(result);
    if (result != VK_SUCCESS)
    {
        return false;
    }
  
    waterComputePass.workGroupDispatchX = params.width / c_numWorkGroupShaderX;
    waterComputePass.workGroupDispatchY = params.width / c_numWorkGroupShaderY;

    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(Game::Get()->GetVulkanPhysicalDevice(), &physicalDeviceProperties);

    if (physicalDeviceProperties.limits.maxComputeWorkGroupCount[0] < waterComputePass.workGroupDispatchX || 
        physicalDeviceProperties.limits.maxComputeWorkGroupCount[1] < waterComputePass.workGroupDispatchY)
    {
        DUCK_DEMO_ASSERT(false);
    }

    return true;
}

void Free_WaterComputePass(WaterComputePass& waterComputePass)
{
    if (waterComputePass.semaphore != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(Game::Get()->GetVulkanDevice(), waterComputePass.semaphore, s_allocator);
    }

    if (waterComputePass.commandBuffer != VK_NULL_HANDLE)
    {
        vkFreeCommandBuffers(Game::Get()->GetVulkanDevice(), waterComputePass.commandPool, 1, &waterComputePass.commandBuffer);
    }

    if (waterComputePass.commandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(Game::Get()->GetVulkanDevice(), waterComputePass.commandPool, s_allocator);
    }

    if (waterComputePass.pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(Game::Get()->GetVulkanDevice(), waterComputePass.pipeline, s_allocator);
    }

    // TODO: A shader module can be destroyed while pipelines created using its shaders are still in use.
    if (waterComputePass.shaderModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(Game::Get()->GetVulkanDevice(), waterComputePass.shaderModule, s_allocator);
    }

    for (VkImageView& imageView : waterComputePass.imageViews)
    {
        if (imageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(Game::Get()->GetVulkanDevice(), imageView, s_allocator);
        }
    }

    for (VkDeviceMemory& deviceMemory : waterComputePass.deviceMemories)
    {
        if (deviceMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(Game::Get()->GetVulkanDevice(), deviceMemory, s_allocator);
        }
    }

    for (VkImage& image : waterComputePass.images)
    {
        if (image != VK_NULL_HANDLE)
        {
            vkDestroyImage(Game::Get()->GetVulkanDevice(), image, s_allocator);
        }
    }

    for (VkDescriptorSet descriptorSet : waterComputePass.descriptorSets)
    {
        if (descriptorSet != VK_NULL_HANDLE)
        {
            vkFreeDescriptorSets(Game::Get()->GetVulkanDevice(), waterComputePass.descriptorPool, 1, &descriptorSet);
        }
    }

    if (waterComputePass.pipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(Game::Get()->GetVulkanDevice(), waterComputePass.pipelineLayout, s_allocator);
    }

    for (VkDescriptorSetLayout& descriptorSetLayout : waterComputePass.descriptorSetLayouts)
    {
        if (descriptorSetLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(Game::Get()->GetVulkanDevice(), descriptorSetLayout, s_allocator);
        }
    }

    if (waterComputePass.descriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(Game::Get()->GetVulkanDevice(), waterComputePass.descriptorPool, s_allocator);
    }
}

void Compute_WaterComputePass(WaterComputePass& waterComputePass)
{
    {
        const uint32_t newImageOffset = waterComputePass.currentImageOffset + 1;
        waterComputePass.currentImageOffset = newImageOffset >= c_waterComputePassTextureCount ? 0 : newImageOffset;
    }

    VkCommandBufferBeginInfo commandBufferBeginInfo;
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.pNext = nullptr;
    commandBufferBeginInfo.flags = 0;
    commandBufferBeginInfo.pInheritanceInfo = nullptr;

    DUCK_DEMO_VULKAN_ASSERT(vkBeginCommandBuffer(waterComputePass.commandBuffer, &commandBufferBeginInfo));

    {
        std::array<VkImageMemoryBarrier, 3> imageMemoryBarriers;
        for (std::size_t i = 0; i < imageMemoryBarriers.size(); ++i)
        {
            VkImageSubresourceRange imageSubresourceRange;
            imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageSubresourceRange.baseMipLevel = 0;
            imageSubresourceRange.levelCount = 1;
            imageSubresourceRange.baseArrayLayer = 0;
            imageSubresourceRange.layerCount = 1;

            VkImageMemoryBarrier& imageMemoryBarrier = imageMemoryBarriers[i];
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.pNext = nullptr;
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_NONE;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            imageMemoryBarrier.srcQueueFamilyIndex = Game::Get()->GetVulkanGraphicsQueueIndex();
            imageMemoryBarrier.dstQueueFamilyIndex = Game::Get()->GetVulkanComputeQueueIndex();
            imageMemoryBarrier.image = waterComputePass.images[i];
            imageMemoryBarrier.subresourceRange = imageSubresourceRange;
        }
        vkCmdPipelineBarrier(waterComputePass.commandBuffer, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, static_cast<uint32_t>(imageMemoryBarriers.size()), imageMemoryBarriers.data());
    }

    vkCmdBindPipeline(waterComputePass.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, waterComputePass.pipeline);

    uint32_t firstIndex = waterComputePass.currentImageOffset;
    uint32_t secondIndex = waterComputePass.currentImageOffset + 1;
    secondIndex = secondIndex >= c_waterComputePassTextureCount ? secondIndex - c_waterComputePassTextureCount : secondIndex;
    uint32_t thirdIndex = waterComputePass.currentImageOffset + 2;
    thirdIndex = thirdIndex >= c_waterComputePassTextureCount ? thirdIndex - c_waterComputePassTextureCount : thirdIndex;

    vkCmdBindDescriptorSets(waterComputePass.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, waterComputePass.pipelineLayout, firstIndex, 1, &waterComputePass.descriptorSets[0], 0, nullptr);
    vkCmdBindDescriptorSets(waterComputePass.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, waterComputePass.pipelineLayout, secondIndex, 1, &waterComputePass.descriptorSets[1], 0, nullptr);
    vkCmdBindDescriptorSets(waterComputePass.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, waterComputePass.pipelineLayout, thirdIndex, 1, &waterComputePass.descriptorSets[2], 0, nullptr);

    vkCmdDispatch(waterComputePass.commandBuffer, waterComputePass.workGroupDispatchX, waterComputePass.workGroupDispatchY, 1);

    {
        std::array<VkImageMemoryBarrier, 3> imageMemoryBarriers;
        for (std::size_t i = 0; i < imageMemoryBarriers.size(); ++i)
        {
            VkImageSubresourceRange imageSubresourceRange;
            imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageSubresourceRange.baseMipLevel = 0;
            imageSubresourceRange.levelCount = 1;
            imageSubresourceRange.baseArrayLayer = 0;
            imageSubresourceRange.layerCount = 1;

            VkImageMemoryBarrier& imageMemoryBarrier = imageMemoryBarriers[i];
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.pNext = nullptr;
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_NONE;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageMemoryBarrier.srcQueueFamilyIndex = Game::Get()->GetVulkanComputeQueueIndex();
            imageMemoryBarrier.dstQueueFamilyIndex = Game::Get()->GetVulkanGraphicsQueueIndex();
            imageMemoryBarrier.image = waterComputePass.images[i];
            imageMemoryBarrier.subresourceRange = imageSubresourceRange;
        }
        vkCmdPipelineBarrier(waterComputePass.commandBuffer, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, static_cast<uint32_t>(imageMemoryBarriers.size()), imageMemoryBarriers.data());
    }

    vkEndCommandBuffer(waterComputePass.commandBuffer);

    VkQueue computeQueue = VK_NULL_HANDLE;
    vkGetDeviceQueue(Game::Get()->GetVulkanDevice(), Game::Get()->GetVulkanComputeQueueIndex(), 0, &computeQueue);

    VkSubmitInfo submitInfo;
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.pWaitDstStageMask = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &waterComputePass.commandBuffer;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = nullptr;

    DUCK_DEMO_VULKAN_ASSERT(vkQueueSubmit(computeQueue, 1, &submitInfo, VK_NULL_HANDLE));
    DUCK_DEMO_VULKAN_ASSERT(vkQueueWaitIdle(computeQueue));
}

VkImageView GetCurrImageView_WaterComputePass(WaterComputePass& waterComputePass)
{
    uint32_t thirdIndex = waterComputePass.currentImageOffset + 2;
    thirdIndex = thirdIndex >= c_waterComputePassTextureCount ? thirdIndex - c_waterComputePassTextureCount : thirdIndex;

    return waterComputePass.imageViews[thirdIndex];
}
