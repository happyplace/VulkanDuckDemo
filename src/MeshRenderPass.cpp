#include "MeshRenderPass.h"

#include "Game.h"
#include "DuckDemoUtils.h"
#include "DuckDemoGame.h"


bool InitFrameBuffers(MeshRenderPass& meshRenderPass);
void DestroyFrameBuffers(MeshRenderPass& meshRenderPass);

bool Init_MeshRenderPass(MeshRenderPass& meshRenderPass, const MeshRenderPassParams& meshRenderPassParams)
{
    VkResult result = VK_SUCCESS;

    std::array<VkAttachmentDescription, 2> attachmentDescriptions;

    attachmentDescriptions[0].flags = 0;
    attachmentDescriptions[0].format = Game::Get()->GetVulkanSwapchainPixelFormat();
    attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_GENERAL;

    attachmentDescriptions[1].flags = 0;
    attachmentDescriptions[1].format = VK_FORMAT_D32_SFLOAT_S8_UINT;
    attachmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colourAttachmentReference;
    colourAttachmentReference.attachment = 0;
    colourAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentReference;
    depthAttachmentReference.attachment = 1;
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription;
    subpassDescription.flags = 0;
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.inputAttachmentCount = 0;
    subpassDescription.pInputAttachments = nullptr;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colourAttachmentReference;
    subpassDescription.pResolveAttachments = nullptr;
    subpassDescription.pDepthStencilAttachment = &depthAttachmentReference;
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments = nullptr;

    VkSubpassDependency subpassDependency;
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass = 0;
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask = 0;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependency.dependencyFlags = 0;

    VkRenderPassCreateInfo renderPassCreateInfo;
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.pNext = nullptr;
    renderPassCreateInfo.flags = 0;
    renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
    renderPassCreateInfo.pAttachments = attachmentDescriptions.data();
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDescription;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &subpassDependency;
    // renderPassCreateInfo.dependencyCount = 0;
    // renderPassCreateInfo.pDependencies = nullptr;

    DUCK_DEMO_VULKAN_ASSERT(vkCreateRenderPass(Game::Get()->GetVulkanDevice(), &renderPassCreateInfo, s_allocator, &meshRenderPass.m_vulkanRenderPass));

    if (!InitFrameBuffers(meshRenderPass))
    {
        return false;
    }

    std::array<VkDescriptorPoolSize, 5> descriptorPoolSize;
    descriptorPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorPoolSize[0].descriptorCount = 1;
    descriptorPoolSize[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descriptorPoolSize[1].descriptorCount = 1;
    descriptorPoolSize[2].type = VK_DESCRIPTOR_TYPE_SAMPLER;
    descriptorPoolSize[2].descriptorCount = 1;
    descriptorPoolSize[3].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    descriptorPoolSize[3].descriptorCount = meshRenderPassParams.m_maxRenderObjectCount;
    descriptorPoolSize[4].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    descriptorPoolSize[4].descriptorCount = 1;

    VkPhysicalDeviceFeatures physicalDeviceFeatures;
    vkGetPhysicalDeviceFeatures(Game::Get()->GetVulkanPhysicalDevice(), &physicalDeviceFeatures);

    VkSamplerCreateInfo samplerCreateInfo;
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.pNext = nullptr;
    samplerCreateInfo.flags = 0;
    samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
    samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCreateInfo.mipLodBias = 0.0f;
    if (physicalDeviceFeatures.samplerAnisotropy)
    {
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(Game::Get()->GetVulkanPhysicalDevice(), &physicalDeviceProperties);

        samplerCreateInfo.anisotropyEnable = VK_TRUE;
        samplerCreateInfo.maxAnisotropy = physicalDeviceProperties.limits.maxSamplerAnisotropy;
    }
    else
    {
        samplerCreateInfo.anisotropyEnable = VK_FALSE;
        samplerCreateInfo.maxAnisotropy = 1.0f;
    }
    samplerCreateInfo.compareEnable = VK_FALSE;
    samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
    samplerCreateInfo.minLod = 0.0f;
    samplerCreateInfo.maxLod = 0.0f;
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
    DUCK_DEMO_VULKAN_ASSERT(vkCreateSampler(Game::Get()->GetVulkanDevice(), &samplerCreateInfo, s_allocator, &meshRenderPass.m_vulkanSampler));

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo;
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.pNext = nullptr;
    descriptorPoolCreateInfo.flags = 0;
    descriptorPoolCreateInfo.maxSets = static_cast<uint32_t>(descriptorPoolSize.size());
    descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSize.size());
    descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSize.data();

    result = vkCreateDescriptorPool(Game::Get()->GetVulkanDevice(), &descriptorPoolCreateInfo, s_allocator, &meshRenderPass.m_vulkanDescriptorPool);
    if (result != VK_SUCCESS)
    {
        DUCK_DEMO_VULKAN_ASSERT(result);
        return false;
    }

    DUCK_DEMO_VULKAN_ASSERT(Game::Get()->CreateVulkanBuffer(static_cast<VkDeviceSize>(sizeof(FrameBuf)),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, meshRenderPass.m_vulkanFrameBuffer));
    DUCK_DEMO_VULKAN_ASSERT(Game::Get()->CreateVulkanBuffer(static_cast<VkDeviceSize>(Game::Get()->CalculateUniformBufferSize(sizeof(ObjectBuf)) * meshRenderPassParams.m_maxRenderObjectCount), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, meshRenderPass.m_vulkanObjectBuffer));

    VkDescriptorSetLayoutBinding frameBufDescriptorSetLayoutBindings;
    frameBufDescriptorSetLayoutBindings.binding = 0;
    frameBufDescriptorSetLayoutBindings.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    frameBufDescriptorSetLayoutBindings.descriptorCount = 1;
    frameBufDescriptorSetLayoutBindings.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    frameBufDescriptorSetLayoutBindings.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding objectBufDescriptorSetLayoutBinding;
    objectBufDescriptorSetLayoutBinding.binding = 0;
    objectBufDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    objectBufDescriptorSetLayoutBinding.descriptorCount = 1;
    objectBufDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    objectBufDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding samplerDescriptorSetLayoutBindings;
    samplerDescriptorSetLayoutBindings.binding = 0;
    samplerDescriptorSetLayoutBindings.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    samplerDescriptorSetLayoutBindings.descriptorCount = 1;
    samplerDescriptorSetLayoutBindings.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerDescriptorSetLayoutBindings.pImmutableSamplers = &meshRenderPass.m_vulkanSampler;

    VkDescriptorSetLayoutBinding sampledImageDescriptorSetLayoutBindings;
    sampledImageDescriptorSetLayoutBindings.binding = 0;
    sampledImageDescriptorSetLayoutBindings.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    sampledImageDescriptorSetLayoutBindings.descriptorCount = meshRenderPassParams.m_maxRenderObjectCount;
    sampledImageDescriptorSetLayoutBindings.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    sampledImageDescriptorSetLayoutBindings.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding waterHeightSampledImageDescriptorSetLayoutBindings;
    waterHeightSampledImageDescriptorSetLayoutBindings.binding = 0;
    waterHeightSampledImageDescriptorSetLayoutBindings.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    waterHeightSampledImageDescriptorSetLayoutBindings.descriptorCount = 1;
    waterHeightSampledImageDescriptorSetLayoutBindings.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    waterHeightSampledImageDescriptorSetLayoutBindings.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo frameBufDescriptorSetLayoutCreateInfo;
    frameBufDescriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    frameBufDescriptorSetLayoutCreateInfo.pNext = nullptr;
    frameBufDescriptorSetLayoutCreateInfo.flags = 0;
    frameBufDescriptorSetLayoutCreateInfo.bindingCount = 1;
    frameBufDescriptorSetLayoutCreateInfo.pBindings = &frameBufDescriptorSetLayoutBindings;

    VkDescriptorSetLayoutCreateInfo objectBufDescriptorSetLayoutCreateInfo;
    objectBufDescriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    objectBufDescriptorSetLayoutCreateInfo.pNext = nullptr;
    objectBufDescriptorSetLayoutCreateInfo.flags = 0;
    objectBufDescriptorSetLayoutCreateInfo.bindingCount = 1;
    objectBufDescriptorSetLayoutCreateInfo.pBindings = &objectBufDescriptorSetLayoutBinding;

    VkDescriptorSetLayoutCreateInfo samplerDescriptorSetLayoutCreateInfo;
    samplerDescriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    samplerDescriptorSetLayoutCreateInfo.pNext = nullptr;
    samplerDescriptorSetLayoutCreateInfo.flags = 0;
    samplerDescriptorSetLayoutCreateInfo.bindingCount = 1;
    samplerDescriptorSetLayoutCreateInfo.pBindings = &samplerDescriptorSetLayoutBindings;

    VkDescriptorSetLayoutCreateInfo sampledImageDescriptorSetLayoutCreateInfo;
    sampledImageDescriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    sampledImageDescriptorSetLayoutCreateInfo.pNext = nullptr;
    sampledImageDescriptorSetLayoutCreateInfo.flags = 0;
    sampledImageDescriptorSetLayoutCreateInfo.bindingCount = 1;
    sampledImageDescriptorSetLayoutCreateInfo.pBindings = &sampledImageDescriptorSetLayoutBindings;

    VkDescriptorSetLayoutCreateInfo waterHeightSampledImageDescriptorSetCreateInfo;
    waterHeightSampledImageDescriptorSetCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    waterHeightSampledImageDescriptorSetCreateInfo.pNext = nullptr;
    waterHeightSampledImageDescriptorSetCreateInfo.flags = 0;
    waterHeightSampledImageDescriptorSetCreateInfo.bindingCount = 1;
    waterHeightSampledImageDescriptorSetCreateInfo.pBindings = &waterHeightSampledImageDescriptorSetLayoutBindings;

    result = vkCreateDescriptorSetLayout(Game::Get()->GetVulkanDevice(), &frameBufDescriptorSetLayoutCreateInfo, s_allocator, &meshRenderPass.m_vulkanDescriptorSetLayouts[0]);
    if (result != VK_SUCCESS)
    {
        DUCK_DEMO_VULKAN_ASSERT(result);
        return false;
    }

    result = vkCreateDescriptorSetLayout(Game::Get()->GetVulkanDevice(), &objectBufDescriptorSetLayoutCreateInfo, s_allocator, &meshRenderPass.m_vulkanDescriptorSetLayouts[1]);
    if (result != VK_SUCCESS)
    {
        DUCK_DEMO_VULKAN_ASSERT(result);
        return false;
    }

    result = vkCreateDescriptorSetLayout(Game::Get()->GetVulkanDevice(), &samplerDescriptorSetLayoutCreateInfo, s_allocator, &meshRenderPass.m_vulkanDescriptorSetLayouts[2]);
    if (result != VK_SUCCESS)
    {
        DUCK_DEMO_VULKAN_ASSERT(result);
        return false;
    }

    result = vkCreateDescriptorSetLayout(Game::Get()->GetVulkanDevice(), &sampledImageDescriptorSetLayoutCreateInfo, s_allocator, &meshRenderPass.m_vulkanDescriptorSetLayouts[3]);
    if (result != VK_SUCCESS)
    {
        DUCK_DEMO_VULKAN_ASSERT(result);
        return false;
    }

    result = vkCreateDescriptorSetLayout(Game::Get()->GetVulkanDevice(), &waterHeightSampledImageDescriptorSetCreateInfo, s_allocator, &meshRenderPass.m_vulkanDescriptorSetLayouts[4]);
    if (result != VK_SUCCESS)
    {
        DUCK_DEMO_VULKAN_ASSERT(result);
        return false;
    }

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.pNext = nullptr;
    descriptorSetAllocateInfo.descriptorPool = meshRenderPass.m_vulkanDescriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = static_cast<uint32_t>(meshRenderPass.m_vulkanDescriptorSetLayouts.size());
    descriptorSetAllocateInfo.pSetLayouts = meshRenderPass.m_vulkanDescriptorSetLayouts.data();

    DUCK_DEMO_VULKAN_ASSERT(vkAllocateDescriptorSets(Game::Get()->GetVulkanDevice(), &descriptorSetAllocateInfo, meshRenderPass.m_vulkanDescriptorSets.data()));

    VkDescriptorBufferInfo frameBufDescriptorBufferInfo;
    frameBufDescriptorBufferInfo.buffer = meshRenderPass.m_vulkanFrameBuffer.m_buffer;
    frameBufDescriptorBufferInfo.offset = 0;
    frameBufDescriptorBufferInfo.range = sizeof(FrameBuf);

    VkDescriptorBufferInfo objectBufDescriptorBufferInfo;
    objectBufDescriptorBufferInfo.buffer = meshRenderPass.m_vulkanObjectBuffer.m_buffer;
    objectBufDescriptorBufferInfo.offset = 0;
    objectBufDescriptorBufferInfo.range = sizeof(ObjectBuf);

    VkDescriptorImageInfo samplerDescriptorImageInfo;
    samplerDescriptorImageInfo.sampler = meshRenderPass.m_vulkanSampler;
    samplerDescriptorImageInfo.imageView = nullptr;
    samplerDescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet frameBufWriteDescriptorSet;
    frameBufWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    frameBufWriteDescriptorSet.pNext = nullptr;
    frameBufWriteDescriptorSet.dstSet = meshRenderPass.m_vulkanDescriptorSets[0];
    frameBufWriteDescriptorSet.dstBinding = 0;
    frameBufWriteDescriptorSet.dstArrayElement = 0;
    frameBufWriteDescriptorSet.descriptorCount = 1;
    frameBufWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    frameBufWriteDescriptorSet.pBufferInfo = &frameBufDescriptorBufferInfo;
    frameBufWriteDescriptorSet.pImageInfo = nullptr;
    frameBufWriteDescriptorSet.pTexelBufferView = nullptr;

    VkWriteDescriptorSet objectBufWriteDescriptorSet;
    objectBufWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    objectBufWriteDescriptorSet.pNext = nullptr;
    objectBufWriteDescriptorSet.dstSet = meshRenderPass.m_vulkanDescriptorSets[1];
    objectBufWriteDescriptorSet.dstBinding = 0;
    objectBufWriteDescriptorSet.dstArrayElement = 0;
    objectBufWriteDescriptorSet.descriptorCount = 1;
    objectBufWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    objectBufWriteDescriptorSet.pBufferInfo = &objectBufDescriptorBufferInfo;
    objectBufWriteDescriptorSet.pImageInfo = nullptr;
    objectBufWriteDescriptorSet.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(Game::Get()->GetVulkanDevice(), 1, &frameBufWriteDescriptorSet, 0, nullptr);
    vkUpdateDescriptorSets(Game::Get()->GetVulkanDevice(), 1, &objectBufWriteDescriptorSet, 0, nullptr);

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.pNext = nullptr;
    pipelineLayoutCreateInfo.flags = 0;
    pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(meshRenderPass.m_vulkanDescriptorSetLayouts.size());
    pipelineLayoutCreateInfo.pSetLayouts = meshRenderPass.m_vulkanDescriptorSetLayouts.data();
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    result = vkCreatePipelineLayout(Game::Get()->GetVulkanDevice(), &pipelineLayoutCreateInfo, s_allocator, &meshRenderPass.m_vulkanPipelineLayout);
    if (result != VK_SUCCESS)
    {
        DUCK_DEMO_VULKAN_ASSERT(result);
        return false;
    }

    {
        shaderc_compile_options_t compileOptions = shaderc_compile_options_initialize();
        if (compileOptions == nullptr)
        {
            DUCK_DEMO_ASSERT(false);
            return false;
        }

        const std::string useWaterTexture = "USE_WATER_TEXTURE";
        shaderc_compile_options_add_macro_definition(compileOptions, useWaterTexture.c_str(), static_cast<size_t>(useWaterTexture.size()), nullptr, 0);

        const std::string duckWaterSample = "DUCK_WATER_SAMPLE";
        shaderc_compile_options_add_macro_definition(compileOptions, duckWaterSample.c_str(), static_cast<size_t>(duckWaterSample.size()), nullptr, 0);

        result = Game::Get()->CompileShaderFromDisk("data/shader_src/MeshShader.vert", shaderc_glsl_vertex_shader, &meshRenderPass.m_vertexShader, compileOptions);
        if (result != VK_SUCCESS)
        {
            DUCK_DEMO_VULKAN_ASSERT(result);
            return false;
        }
    }
    
    shaderc_compile_options_t compileOptions = shaderc_compile_options_initialize();
    if (compileOptions == nullptr)
    {
        DUCK_DEMO_ASSERT(false);
        return false;
    }

    if (meshRenderPassParams.m_wireframe)
    {
        std::string isWireframe = "IS_WIREFRAME";
        shaderc_compile_options_add_macro_definition(compileOptions, isWireframe.c_str(), static_cast<size_t>(isWireframe.size()), nullptr, 0);
    }

    const std::string useDirectionalLight = "USE_DIRECTIONAL_LIGHT";
    shaderc_compile_options_add_macro_definition(compileOptions, useDirectionalLight.c_str(), static_cast<size_t>(useDirectionalLight.size()), nullptr, 0);

    const std::string useSpotLight = "USE_SPOT_LIGHT";
    //shaderc_compile_options_add_macro_definition(compileOptions, useSpotLight.c_str(), static_cast<size_t>(useSpotLight.size()), nullptr, 0);

    const std::string usePointLight = "USE_POINT_LIGHT";
    //shaderc_compile_options_add_macro_definition(compileOptions, usePointLight.c_str(), static_cast<size_t>(usePointLight.size()), nullptr, 0);

    const std::string useTexture = "USE_TEXTURE";
    shaderc_compile_options_add_macro_definition(compileOptions, useTexture.c_str(), static_cast<size_t>(useTexture.size()), nullptr, 0);

    const std::string maxSampledTextureCount = "MAX_SAMPLED_TEXTURE_COUNT";
    const std::string maxSampledTextureCountValue = std::to_string(meshRenderPassParams.m_maxRenderObjectCount);
    shaderc_compile_options_add_macro_definition(compileOptions, 
        maxSampledTextureCount.c_str(), static_cast<size_t>(maxSampledTextureCount.size()), 
        maxSampledTextureCountValue.c_str(),  static_cast<size_t>(maxSampledTextureCountValue.size()));

    result = Game::Get()->CompileShaderFromDisk("data/shader_src/MeshShader.frag", shaderc_glsl_fragment_shader, &meshRenderPass.m_fragmentShader, compileOptions);
    if (result != VK_SUCCESS)
    {
        DUCK_DEMO_VULKAN_ASSERT(result);
        return false;
    }

    std::array<VkPipelineShaderStageCreateInfo, 2> pipelineShaderStageCreateInfo;
    pipelineShaderStageCreateInfo[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineShaderStageCreateInfo[0].pNext = nullptr;
    pipelineShaderStageCreateInfo[0].flags = 0;
    pipelineShaderStageCreateInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    pipelineShaderStageCreateInfo[0].module = meshRenderPass.m_vertexShader;
    pipelineShaderStageCreateInfo[0].pName = "main";
    pipelineShaderStageCreateInfo[0].pSpecializationInfo = nullptr;

    pipelineShaderStageCreateInfo[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineShaderStageCreateInfo[1].pNext = nullptr;
    pipelineShaderStageCreateInfo[1].flags = 0;
    pipelineShaderStageCreateInfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    pipelineShaderStageCreateInfo[1].module = meshRenderPass.m_fragmentShader;
    pipelineShaderStageCreateInfo[1].pName = "main";
    pipelineShaderStageCreateInfo[1].pSpecializationInfo = nullptr;

    std::array<VkVertexInputAttributeDescription, 3> vertexInputAttributeDescriptions;
    vertexInputAttributeDescriptions[0].location = 0;
    vertexInputAttributeDescriptions[0].binding = 0;
    vertexInputAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInputAttributeDescriptions[0].offset = offsetof(MeshLoader::Vertex, position);

    vertexInputAttributeDescriptions[1].location = 1;
    vertexInputAttributeDescriptions[1].binding = 0;
    vertexInputAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInputAttributeDescriptions[1].offset = offsetof(MeshLoader::Vertex, normal);
    
    vertexInputAttributeDescriptions[2].location = 2;
    vertexInputAttributeDescriptions[2].binding = 0;
    vertexInputAttributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    vertexInputAttributeDescriptions[2].offset = offsetof(MeshLoader::Vertex, texture);

    VkVertexInputBindingDescription vertexInputBindingDescription;
    vertexInputBindingDescription.binding = 0;
    vertexInputBindingDescription.stride = sizeof(MeshLoader::Vertex);
    vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo;
    pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipelineVertexInputStateCreateInfo.pNext = nullptr;
    pipelineVertexInputStateCreateInfo.flags = 0;
    pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
    pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexInputBindingDescription;
    pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributeDescriptions.size());
    pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexInputAttributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo;
    pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pipelineInputAssemblyStateCreateInfo.pNext = nullptr;
    pipelineInputAssemblyStateCreateInfo.flags = 0;
    pipelineInputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

    VkPipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo;
    pipelineTessellationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    pipelineTessellationStateCreateInfo.pNext = nullptr;
    pipelineTessellationStateCreateInfo.flags = 0;
    pipelineTessellationStateCreateInfo.patchControlPoints = 0;

    VkViewport viewport;
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = static_cast<float>(Game::Get()->GetVulkanSwapchainWidth());
    viewport.height = static_cast<float>(Game::Get()->GetVulkanSwapchainHeight());
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissoring;
    scissoring.offset.x = 0;
    scissoring.offset.y = 0;
    scissoring.extent.width = Game::Get()->GetVulkanSwapchainWidth();
    scissoring.extent.height = Game::Get()->GetVulkanSwapchainHeight();

    VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo;
    pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pipelineViewportStateCreateInfo.pNext = nullptr;
    pipelineViewportStateCreateInfo.flags = 0;
    pipelineViewportStateCreateInfo.viewportCount = 1;
    pipelineViewportStateCreateInfo.pViewports = &viewport;
    pipelineViewportStateCreateInfo.scissorCount = 1;
    pipelineViewportStateCreateInfo.pScissors = &scissoring;

    VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo;
    pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pipelineRasterizationStateCreateInfo.pNext = nullptr;
    pipelineRasterizationStateCreateInfo.flags = 0;
    pipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
    pipelineRasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    pipelineRasterizationStateCreateInfo.polygonMode = meshRenderPassParams.m_wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
    pipelineRasterizationStateCreateInfo.cullMode = meshRenderPassParams.m_wireframe ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;
    pipelineRasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    pipelineRasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
    pipelineRasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
    pipelineRasterizationStateCreateInfo.depthBiasClamp = 0.0f;
    pipelineRasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;
    pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo;
    pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipelineMultisampleStateCreateInfo.pNext = nullptr;
    pipelineMultisampleStateCreateInfo.flags = 0;
    pipelineMultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pipelineMultisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
    pipelineMultisampleStateCreateInfo.minSampleShading = 0;
    pipelineMultisampleStateCreateInfo.pSampleMask = nullptr;
    pipelineMultisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
    pipelineMultisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState;
    pipelineColorBlendAttachmentState.blendEnable = meshRenderPassParams.m_transparencyBlending ? VK_TRUE : VK_FALSE;
    pipelineColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    pipelineColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    pipelineColorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    pipelineColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    pipelineColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    pipelineColorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
    pipelineColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo;
    pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pipelineColorBlendStateCreateInfo.pNext = nullptr;
    pipelineColorBlendStateCreateInfo.flags = 0;
    pipelineColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
    pipelineColorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
    pipelineColorBlendStateCreateInfo.attachmentCount = 1;
    pipelineColorBlendStateCreateInfo.pAttachments = &pipelineColorBlendAttachmentState;
    pipelineColorBlendStateCreateInfo.blendConstants[0] = 0.0f;
    pipelineColorBlendStateCreateInfo.blendConstants[1] = 0.0f;
    pipelineColorBlendStateCreateInfo.blendConstants[2] = 0.0f;
    pipelineColorBlendStateCreateInfo.blendConstants[3] = 0.0f;

    std::array<VkDynamicState,2> dynamicState;
    dynamicState[0] = VK_DYNAMIC_STATE_VIEWPORT;
    dynamicState[1] = VK_DYNAMIC_STATE_SCISSOR;

    VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo;
    pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    pipelineDynamicStateCreateInfo.pNext = nullptr;
    pipelineDynamicStateCreateInfo.flags = 0;
    pipelineDynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicState.size());
    pipelineDynamicStateCreateInfo.pDynamicStates = dynamicState.data();
    //pipelineDynamicStateCreateInfo.dynamicStateCount = 0;
    //pipelineDynamicStateCreateInfo.pDynamicStates = nullptr;

    VkStencilOpState stencilOpStateFront;
    stencilOpStateFront.failOp = VK_STENCIL_OP_KEEP;
    stencilOpStateFront.passOp = VK_STENCIL_OP_REPLACE;
    stencilOpStateFront.depthFailOp = VK_STENCIL_OP_KEEP;
    stencilOpStateFront.compareOp = VK_COMPARE_OP_GREATER;
    stencilOpStateFront.compareMask = ~0;
    stencilOpStateFront.writeMask = ~0;
    stencilOpStateFront.reference = 1;

    VkStencilOpState stencilOpStateBack;
    stencilOpStateBack.failOp = VK_STENCIL_OP_KEEP;
    stencilOpStateBack.passOp = VK_STENCIL_OP_KEEP;
    stencilOpStateBack.depthFailOp = VK_STENCIL_OP_KEEP;
    stencilOpStateBack.compareOp = VK_COMPARE_OP_NEVER;
    stencilOpStateBack.compareMask = ~0;
    stencilOpStateBack.writeMask = ~0;
    stencilOpStateBack.reference = 0;

    VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo;
    pipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pipelineDepthStencilStateCreateInfo.pNext = nullptr;
    pipelineDepthStencilStateCreateInfo.flags = 0;
    pipelineDepthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
    pipelineDepthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
    pipelineDepthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    pipelineDepthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
    pipelineDepthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
    pipelineDepthStencilStateCreateInfo.front = stencilOpStateFront;
    pipelineDepthStencilStateCreateInfo.back = stencilOpStateBack;
    pipelineDepthStencilStateCreateInfo.minDepthBounds = 0.0f;
    pipelineDepthStencilStateCreateInfo.maxDepthBounds = 1.0f;

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo;
    graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineCreateInfo.pNext = nullptr;
    graphicsPipelineCreateInfo.flags = 0;
    graphicsPipelineCreateInfo.stageCount = static_cast<uint32_t>(pipelineShaderStageCreateInfo.size());
    graphicsPipelineCreateInfo.pStages = pipelineShaderStageCreateInfo.data();
    graphicsPipelineCreateInfo.pVertexInputState = &pipelineVertexInputStateCreateInfo;
    graphicsPipelineCreateInfo.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo;
    graphicsPipelineCreateInfo.pTessellationState = &pipelineTessellationStateCreateInfo;
    graphicsPipelineCreateInfo.pViewportState = &pipelineViewportStateCreateInfo;
    graphicsPipelineCreateInfo.pRasterizationState = &pipelineRasterizationStateCreateInfo;
    graphicsPipelineCreateInfo.pMultisampleState = &pipelineMultisampleStateCreateInfo;
    graphicsPipelineCreateInfo.pDepthStencilState = &pipelineDepthStencilStateCreateInfo;
    graphicsPipelineCreateInfo.pColorBlendState = &pipelineColorBlendStateCreateInfo;
    graphicsPipelineCreateInfo.pDynamicState = &pipelineDynamicStateCreateInfo;
    graphicsPipelineCreateInfo.layout = meshRenderPass.m_vulkanPipelineLayout;
    graphicsPipelineCreateInfo.renderPass = meshRenderPass.m_vulkanRenderPass;
    graphicsPipelineCreateInfo.subpass = 0;
    graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    graphicsPipelineCreateInfo.basePipelineIndex = 0;

    result = vkCreateGraphicsPipelines(Game::Get()->GetVulkanDevice(), VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, s_allocator, &meshRenderPass.m_vulkanPipeline);
    if (result != VK_SUCCESS)
    {
        DUCK_DEMO_VULKAN_ASSERT(result);
        return false;
    }

    return true;
}

void Free_MeshRenderPass(MeshRenderPass& meshRenderPass)
{
    meshRenderPass.m_vulkanFrameBuffer.Reset();
    meshRenderPass.m_vulkanObjectBuffer.Reset();

    if (meshRenderPass.m_vulkanSampler)
    {
        vkDestroySampler(Game::Get()->GetVulkanDevice(), meshRenderPass.m_vulkanSampler, s_allocator);
    }

    if (meshRenderPass.m_vulkanPipeline)
    {
        vkDestroyPipeline(Game::Get()->GetVulkanDevice(), meshRenderPass.m_vulkanPipeline, s_allocator);
    }

    if (meshRenderPass.m_vertexShader)
    {
        vkDestroyShaderModule(Game::Get()->GetVulkanDevice(), meshRenderPass.m_vertexShader, s_allocator);
    }

    if (meshRenderPass.m_fragmentShader)
    {
        vkDestroyShaderModule(Game::Get()->GetVulkanDevice(), meshRenderPass.m_fragmentShader, s_allocator);
    }

    if (meshRenderPass.m_vulkanPipelineLayout)
    {
        vkDestroyPipelineLayout(Game::Get()->GetVulkanDevice(), meshRenderPass.m_vulkanPipelineLayout, s_allocator);
    }

    for (std::size_t i = 0; i < meshRenderPass.m_vulkanDescriptorSetLayouts.size(); ++i)
    {
        if (meshRenderPass.m_vulkanDescriptorSetLayouts[i])
        {
            vkDestroyDescriptorSetLayout(Game::Get()->GetVulkanDevice(), meshRenderPass.m_vulkanDescriptorSetLayouts[i], s_allocator);
        }
    }

    if (meshRenderPass.m_vulkanDescriptorPool)
    {
        vkDestroyDescriptorPool(Game::Get()->GetVulkanDevice(), meshRenderPass.m_vulkanDescriptorPool, s_allocator);
    }

    DestroyFrameBuffers(meshRenderPass);

    if (meshRenderPass.m_vulkanRenderPass)
    {
        vkDestroyRenderPass(Game::Get()->GetVulkanDevice(), meshRenderPass.m_vulkanRenderPass, s_allocator);
    }
}

bool InitFrameBuffers(MeshRenderPass& meshRenderPass)
{
    meshRenderPass.m_vulkanFrameBuffers.clear();

    std::array<VkImageView, 2> attachments;
    attachments[1] = Game::Get()->GetVulkanDepthStencilImageView();

    VkFramebufferCreateInfo frameBufferCreateInfo;
    frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCreateInfo.pNext = nullptr;
    frameBufferCreateInfo.flags = 0;
    frameBufferCreateInfo.renderPass = meshRenderPass.m_vulkanRenderPass;
    frameBufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    frameBufferCreateInfo.pAttachments = attachments.data();
    frameBufferCreateInfo.width = Game::Get()->GetVulkanSwapchainWidth();
    frameBufferCreateInfo.height = Game::Get()->GetVulkanSwapchainHeight();
    frameBufferCreateInfo.layers = 1;

    for (VkImageView imageView : Game::Get()->GetVulkanSwapchainImageViews())
    {
        attachments[0] = imageView;

        VkFramebuffer framebuffer;
        DUCK_DEMO_VULKAN_ASSERT(vkCreateFramebuffer(Game::Get()->GetVulkanDevice(), &frameBufferCreateInfo, s_allocator, &framebuffer));

        meshRenderPass.m_vulkanFrameBuffers.push_back(framebuffer);
    }

    return true;
}

void DestroyFrameBuffers(MeshRenderPass& meshRenderPass)
{
    for (VkFramebuffer frameBuffer : meshRenderPass.m_vulkanFrameBuffers)
    {
        vkDestroyFramebuffer(Game::Get()->GetVulkanDevice(), frameBuffer, s_allocator);
    }
}

void Resize_MeshRenderPass(MeshRenderPass& meshRenderPass)
{
    DestroyFrameBuffers(meshRenderPass);
    InitFrameBuffers(meshRenderPass);
}

void Render_MeshRenderPass(MeshRenderPass& meshRenderPass, VkCommandBuffer commandBuffer, const std::vector<RenderObject>& renderObjects)
{
    std::array<VkClearValue, 2> clearValues;
    clearValues[0] = Game::Get()->GetVulkanClearValue();
    clearValues[1].depthStencil.depth = 1.0f;
    clearValues[1].depthStencil.stencil = 0u;

    VkRenderPassBeginInfo renderPassBeginInfo;
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.pNext = nullptr;
    renderPassBeginInfo.renderPass = meshRenderPass.m_vulkanRenderPass;
    renderPassBeginInfo.framebuffer = meshRenderPass.m_vulkanFrameBuffers[Game::Get()->GetCurrentSwapchainImageIndex()];
    renderPassBeginInfo.renderArea.extent.width = Game::Get()->GetVulkanSwapchainWidth();
    renderPassBeginInfo.renderArea.extent.height = Game::Get()->GetVulkanSwapchainHeight();
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassBeginInfo.pClearValues = clearValues.data();
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshRenderPass.m_vulkanPipeline);

    VkViewport viewport;
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = static_cast<float>(Game::Get()->GetVulkanSwapchainWidth());
    viewport.height = static_cast<float>(Game::Get()->GetVulkanSwapchainHeight());
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = Game::Get()->GetVulkanSwapchainWidth();
    scissor.extent.height = Game::Get()->GetVulkanSwapchainHeight();
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshRenderPass.m_vulkanPipelineLayout, 0, 1, &meshRenderPass.m_vulkanDescriptorSets[0], 0, nullptr);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshRenderPass.m_vulkanPipelineLayout, 2, 1, &meshRenderPass.m_vulkanDescriptorSets[2], 0, nullptr);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshRenderPass.m_vulkanPipelineLayout, 3, 1, &meshRenderPass.m_vulkanDescriptorSets[3], 0, nullptr);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshRenderPass.m_vulkanPipelineLayout, 4, 1, &meshRenderPass.m_vulkanDescriptorSets[4], 0, nullptr);

    for (const RenderObject& renderObject : renderObjects)
    {
        uint32_t dynamicOffsets = Game::Get()->CalculateUniformBufferSize(sizeof(renderObject.objectBuf)) * renderObject.objectBufferIndex;
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshRenderPass.m_vulkanPipelineLayout, 1, 1, &meshRenderPass.m_vulkanDescriptorSets[1], 1, &dynamicOffsets);

        const VkDeviceSize vertexOffset = 0;
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &renderObject.m_vertexBuffer->m_buffer, &vertexOffset);
        vkCmdBindIndexBuffer(commandBuffer, renderObject.m_indexBuffer->m_buffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(commandBuffer, renderObject.m_indexCount, 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(commandBuffer);
}

void SetWaterImageView_MeshRenderPass(MeshRenderPass& meshRenderPass, VkImageView imageView)
{
    VkDescriptorImageInfo descriptorImageInfo;
    descriptorImageInfo.sampler = nullptr;
    descriptorImageInfo.imageView = imageView;
    descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet sampledImageWriteDescriptorSet;
    sampledImageWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    sampledImageWriteDescriptorSet.pNext = nullptr;
    sampledImageWriteDescriptorSet.dstSet = meshRenderPass.m_vulkanDescriptorSets[4];
    sampledImageWriteDescriptorSet.dstBinding = 0;
    sampledImageWriteDescriptorSet.dstArrayElement = 0;
    sampledImageWriteDescriptorSet.descriptorCount = 1;
    sampledImageWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    sampledImageWriteDescriptorSet.pBufferInfo = nullptr;
    sampledImageWriteDescriptorSet.pImageInfo = &descriptorImageInfo;
    sampledImageWriteDescriptorSet.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(Game::Get()->GetVulkanDevice(), 1, &sampledImageWriteDescriptorSet, 0, nullptr);
}

