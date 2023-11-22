#include "DuckDemoGame.h"

DuckDemoGame::DuckDemoGame()
{
    DUCK_DEMO_ASSERT(!ms_instance);
    ms_instance = this;
}

DuckDemoGame::~DuckDemoGame()
{
    m_vulkanFrameBuffer.Reset();
    m_vulkanObjectBuffer.Reset();

    if (m_vulkanPipeline)
    {
        vkDestroyPipeline(m_vulkanDevice, m_vulkanPipeline, s_allocator);
    }

    if (m_vertexShader)
    {
        vkDestroyShaderModule(m_vulkanDevice, m_vertexShader, s_allocator);
    }

    if (m_fragmentShader)
    {
        vkDestroyShaderModule(m_vulkanDevice, m_fragmentShader, s_allocator);
    }

    if (m_vulkanPipelineLayout)
    {
        vkDestroyPipelineLayout(m_vulkanDevice, m_vulkanPipelineLayout, s_allocator);
    }

    for (VkDescriptorSetLayout descriptorSetLayout : m_vulkanDescriptorSetLayouts)
    {
        if (descriptorSetLayout)
        {
            vkDestroyDescriptorSetLayout(m_vulkanDevice, descriptorSetLayout, s_allocator);
        }
    }

    if (m_vulkanDescriptorPool)
    {
        vkDestroyDescriptorPool(m_vulkanDevice, m_vulkanDescriptorPool, s_allocator);
    }

    DestroyFrameBuffers();

    if (m_vulkanRenderPass)
    {
        vkDestroyRenderPass(m_vulkanDevice, m_vulkanRenderPass, s_allocator);
    }

    DUCK_DEMO_ASSERT(ms_instance == this);
    ms_instance = nullptr;
}

bool DuckDemoGame::OnInit()
{
    VkAttachmentDescription attachmentDescription;
    attachmentDescription.flags = 0;
    attachmentDescription.format = m_vulkanSwapchainPixelFormat;
    attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colourAttachmentReference;
    colourAttachmentReference.attachment = 0;
    colourAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription;
    subpassDescription.flags = 0;
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.inputAttachmentCount = 0;
    subpassDescription.pInputAttachments = nullptr;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colourAttachmentReference;
    subpassDescription.pResolveAttachments = nullptr;
    subpassDescription.pDepthStencilAttachment = nullptr;
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
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &attachmentDescription;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDescription;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &subpassDependency;

    DUCK_DEMO_VULKAN_ASSERT(vkCreateRenderPass(m_vulkanDevice, &renderPassCreateInfo, s_allocator, &m_vulkanRenderPass));

    if (!InitFrameBuffers())
    {
        return false;
    }

    VkDescriptorPoolSize descriptorPoolSize[2];
    descriptorPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorPoolSize[0].descriptorCount = 1;
    descriptorPoolSize[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorPoolSize[1].descriptorCount = 1;

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo;
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.pNext = nullptr;
    descriptorPoolCreateInfo.flags = 0;
    descriptorPoolCreateInfo.maxSets = 4;
    descriptorPoolCreateInfo.poolSizeCount = 4;
    descriptorPoolCreateInfo.pPoolSizes = &descriptorPoolSize[0];

    VkResult result = vkCreateDescriptorPool(m_vulkanDevice, &descriptorPoolCreateInfo, s_allocator, &m_vulkanDescriptorPool);
    if (result != VK_SUCCESS)
    {
        DUCK_DEMO_VULKAN_ASSERT(result);
        return false;
    }

    VkDescriptorSetLayoutBinding frameBuf;
    frameBuf.binding = 0;
    frameBuf.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    frameBuf.descriptorCount = 1;
    frameBuf.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    frameBuf.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding objectBuf;
    objectBuf.binding = 0;
    objectBuf.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    objectBuf.descriptorCount = 1;
    objectBuf.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    objectBuf.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo0;
    descriptorSetLayoutCreateInfo0.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo0.pNext = nullptr;
    descriptorSetLayoutCreateInfo0.flags = 0;
    descriptorSetLayoutCreateInfo0.bindingCount = 1;
    descriptorSetLayoutCreateInfo0.pBindings = &frameBuf;

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo1;
    descriptorSetLayoutCreateInfo1.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo1.pNext = nullptr;
    descriptorSetLayoutCreateInfo1.flags = 0;
    descriptorSetLayoutCreateInfo1.bindingCount = 1;
    descriptorSetLayoutCreateInfo1.pBindings = &objectBuf;

    result = vkCreateDescriptorSetLayout(m_vulkanDevice, &descriptorSetLayoutCreateInfo0, s_allocator, &m_vulkanDescriptorSetLayouts[0]);
    if (result != VK_SUCCESS)
    {
        DUCK_DEMO_VULKAN_ASSERT(result);
        return false;
    }

    result = vkCreateDescriptorSetLayout(m_vulkanDevice, &descriptorSetLayoutCreateInfo1, s_allocator, &m_vulkanDescriptorSetLayouts[1]);
    if (result != VK_SUCCESS)
    {
        DUCK_DEMO_VULKAN_ASSERT(result);
        return false;
    }

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.pNext = nullptr;
    descriptorSetAllocateInfo.descriptorPool = m_vulkanDescriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = m_vulkanDescriptorSetLayouts.size();
    descriptorSetAllocateInfo.pSetLayouts = m_vulkanDescriptorSetLayouts.data();

    DUCK_DEMO_VULKAN_ASSERT(vkAllocateDescriptorSets(m_vulkanDevice, &descriptorSetAllocateInfo, m_vulkanDescriptorSets.data()));

    DUCK_DEMO_VULKAN_ASSERT(CreateVulkanBuffer(static_cast<VkDeviceSize>(sizeof(FrameBuf)),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, m_vulkanFrameBuffer));
    DUCK_DEMO_VULKAN_ASSERT(CreateVulkanBuffer(static_cast<VkDeviceSize>(sizeof(ObjectBuf)),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, m_vulkanObjectBuffer));

    VkDescriptorBufferInfo descriptorBufferInfo[2];
    
    descriptorBufferInfo[0].buffer = m_vulkanFrameBuffer.m_buffer;
    descriptorBufferInfo[0].offset = 0;
    descriptorBufferInfo[0].range = sizeof(FrameBuf);

    descriptorBufferInfo[1].buffer = m_vulkanObjectBuffer.m_buffer;
    descriptorBufferInfo[1].offset = 0;
    descriptorBufferInfo[1].range = sizeof(ObjectBuf);

    std::array<VkWriteDescriptorSet, 2> writeDescriptorSet;

    writeDescriptorSet[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[0].pNext = nullptr;
    writeDescriptorSet[0].dstSet = m_vulkanDescriptorSets[0];
    writeDescriptorSet[0].dstBinding = 0;
    writeDescriptorSet[0].dstArrayElement = 0;
    writeDescriptorSet[0].descriptorCount = 1;
    writeDescriptorSet[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSet[0].pBufferInfo = &descriptorBufferInfo[0];
    writeDescriptorSet[0].pImageInfo = nullptr;
    writeDescriptorSet[0].pTexelBufferView = nullptr;

    writeDescriptorSet[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[1].pNext = nullptr;
    writeDescriptorSet[1].dstSet = m_vulkanDescriptorSets[1];
    writeDescriptorSet[1].dstBinding = 0;
    writeDescriptorSet[1].dstArrayElement = 0;
    writeDescriptorSet[1].descriptorCount = 1;
    writeDescriptorSet[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSet[1].pBufferInfo = &descriptorBufferInfo[1];
    writeDescriptorSet[1].pImageInfo = nullptr;
    writeDescriptorSet[1].pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(m_vulkanDevice, static_cast<uint32_t>(writeDescriptorSet.size()), writeDescriptorSet.data(), 0, nullptr);

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.pNext = nullptr;
    pipelineLayoutCreateInfo.flags = 0;
    pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(m_vulkanDescriptorSetLayouts.size());
    pipelineLayoutCreateInfo.pSetLayouts = &m_vulkanDescriptorSetLayouts.data()[0];
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    result = vkCreatePipelineLayout(m_vulkanDevice, &pipelineLayoutCreateInfo, s_allocator, &m_vulkanPipelineLayout);
    if (result != VK_SUCCESS)
    {
        DUCK_DEMO_VULKAN_ASSERT(result);
        return false;
    }

    result = CompileShaderFromDisk("data/shader_src/MeshShader.vert", shaderc_glsl_vertex_shader, &m_vertexShader);
    if (result != VK_SUCCESS)
    {
        DUCK_DEMO_VULKAN_ASSERT(result);
        return false;
    }
    
    result = CompileShaderFromDisk("data/shader_src/MeshShader.frag", shaderc_glsl_fragment_shader, &m_fragmentShader);
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
    pipelineShaderStageCreateInfo[0].module = m_vertexShader;
    pipelineShaderStageCreateInfo[0].pName = "main";
    pipelineShaderStageCreateInfo[0].pSpecializationInfo = nullptr;

    pipelineShaderStageCreateInfo[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineShaderStageCreateInfo[1].pNext = nullptr;
    pipelineShaderStageCreateInfo[1].flags = 0;
    pipelineShaderStageCreateInfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    pipelineShaderStageCreateInfo[1].module = m_fragmentShader;
    pipelineShaderStageCreateInfo[1].pName = "main";
    pipelineShaderStageCreateInfo[1].pSpecializationInfo = nullptr;

    VkVertexInputAttributeDescription vertexInputAttributeDescription;
    vertexInputAttributeDescription.location = 0;
    vertexInputAttributeDescription.binding = 0;
    vertexInputAttributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInputAttributeDescription.offset = offsetof(Vertex, aPosition);

    VkVertexInputBindingDescription vertexInputBindingDescription;
    vertexInputBindingDescription.binding = 0;
    vertexInputBindingDescription.stride = sizeof(Vertex);
    vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo;
    pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipelineVertexInputStateCreateInfo.pNext = nullptr;
    pipelineVertexInputStateCreateInfo.flags = 0;
    pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
    pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexInputBindingDescription;
    pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = 1;
    pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = &vertexInputAttributeDescription;

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
    viewport.width = static_cast<float>(m_vulkanSwapchainWidth);
    viewport.height = static_cast<float>(m_vulkanSwapchainHeight);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissoring;
    scissoring.offset.x = 0;
    scissoring.offset.y = 0;
    scissoring.extent.width = m_vulkanSwapchainWidth;
    scissoring.extent.height = m_vulkanSwapchainHeight;

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
    pipelineRasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    pipelineRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_NONE;
    pipelineRasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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
    pipelineColorBlendAttachmentState.blendEnable = VK_FALSE;
    pipelineColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
    pipelineColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
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

    // std::array<VkDynamicState,2> dynamicState;
    // dynamicState[0] = VK_DYNAMIC_STATE_VIEWPORT;
    // dynamicState[1] = VK_DYNAMIC_STATE_SCISSOR;

    VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo;
    pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    pipelineDynamicStateCreateInfo.pNext = nullptr;
    pipelineDynamicStateCreateInfo.flags = 0;
    //pipelineDynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicState.size());
    //pipelineDynamicStateCreateInfo.pDynamicStates = dynamicState.data();
    pipelineDynamicStateCreateInfo.dynamicStateCount = 0;
    pipelineDynamicStateCreateInfo.pDynamicStates = nullptr;

    VkStencilOpState stencilOpStateFront;
    stencilOpStateFront.failOp = VK_STENCIL_OP_KEEP;
    stencilOpStateFront.passOp = VK_STENCIL_OP_KEEP;
    stencilOpStateFront.depthFailOp = VK_STENCIL_OP_KEEP;
    stencilOpStateFront.compareOp = VK_COMPARE_OP_NEVER;
    stencilOpStateFront.compareMask = ~0;
    stencilOpStateFront.writeMask = ~0;
    stencilOpStateFront.reference = 0;

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
    graphicsPipelineCreateInfo.layout = m_vulkanPipelineLayout;
    graphicsPipelineCreateInfo.renderPass = m_vulkanRenderPass;
    graphicsPipelineCreateInfo.subpass = 0;
    graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    graphicsPipelineCreateInfo.basePipelineIndex = 0;

    result = vkCreateGraphicsPipelines(m_vulkanDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, s_allocator, &m_vulkanPipeline);
    if (result != VK_SUCCESS)
    {
        DUCK_DEMO_VULKAN_ASSERT(result);
        return false;
    }

    return true;
}

bool DuckDemoGame::InitFrameBuffers()
{
    m_vulkanSwapchainFrameBuffers.clear();

    VkFramebufferCreateInfo frameBufferCreateInfo;
    frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCreateInfo.pNext = nullptr;
    frameBufferCreateInfo.flags = 0;
    frameBufferCreateInfo.renderPass = m_vulkanRenderPass;
    frameBufferCreateInfo.width = m_vulkanSwapchainWidth;
    frameBufferCreateInfo.height = m_vulkanSwapchainHeight;
    frameBufferCreateInfo.layers = 1;

    for (VkImageView imageView : m_vulkanSwapchainImageViews)
    {
        frameBufferCreateInfo.attachmentCount = 1;
        frameBufferCreateInfo.pAttachments = &imageView;

        VkFramebuffer framebuffer;
        DUCK_DEMO_VULKAN_ASSERT(vkCreateFramebuffer(m_vulkanDevice, &frameBufferCreateInfo, s_allocator, &framebuffer));

        m_vulkanSwapchainFrameBuffers.push_back(framebuffer);
    }

    return true;
}

void DuckDemoGame::DestroyFrameBuffers()
{
    for (VkFramebuffer frameBuffer : m_vulkanSwapchainFrameBuffers)
    {
        vkDestroyFramebuffer(m_vulkanDevice, frameBuffer, s_allocator);
    }
}

void DuckDemoGame::OnResize()
{
    DestroyFrameBuffers();
    InitFrameBuffers();
}

void DuckDemoGame::OnUpdate(const GameTimer& gameTimer)
{

}

void DuckDemoGame::OnRender()
{
    VkRenderPassBeginInfo renderPassBeginInfo;
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.pNext = nullptr;
    renderPassBeginInfo.renderPass = m_vulkanRenderPass;
    renderPassBeginInfo.framebuffer = m_vulkanSwapchainFrameBuffers[m_currentSwapchainImageIndex];
    renderPassBeginInfo.renderArea.extent.width = m_vulkanSwapchainWidth;
    renderPassBeginInfo.renderArea.extent.height = m_vulkanSwapchainHeight;
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &m_vulkanClearValue;
    vkCmdBeginRenderPass(m_vulkanPrimaryCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport;
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = static_cast<float>(m_vulkanSwapchainWidth);
    viewport.height = static_cast<float>(m_vulkanSwapchainHeight);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(m_vulkanPrimaryCommandBuffer, 0, 1, &viewport);

    VkRect2D scissor;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = m_vulkanSwapchainWidth;
    scissor.extent.height = m_vulkanSwapchainHeight;
    vkCmdSetScissor(m_vulkanPrimaryCommandBuffer, 0, 1, &scissor);

    vkCmdEndRenderPass(m_vulkanPrimaryCommandBuffer);
}
