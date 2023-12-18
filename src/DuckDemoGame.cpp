#include "DuckDemoGame.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/glm.hpp"
#include "glm/gtx/euler_angles.hpp"

#include "meshloader/MeshLoader.h"

DuckDemoGame::DuckDemoGame()
{
    DUCK_DEMO_ASSERT(!ms_instance);
    ms_instance = this;
}

DuckDemoGame::~DuckDemoGame()
{
    m_vulkanFrameBuffer.Reset();
    m_vulkanObjectBuffer.Reset();
    m_vulkanDuckIndexBuffer.Reset();
    m_vulkanDuckVertexBuffer.Reset();
    m_vulkanFloorIndexBuffer.Reset();
    m_vulkanFloorVertexBuffer.Reset();

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

    for (std::size_t i = 0; i < m_vulkanDescriptorSetLayouts.size(); ++i)
    {
        if (m_vulkanDescriptorSetLayouts[i])
        {
            vkDestroyDescriptorSetLayout(m_vulkanDevice, m_vulkanDescriptorSetLayouts[i], s_allocator);
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

    std::array<VkDescriptorPoolSize, 2> descriptorPoolSize;
    descriptorPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorPoolSize[0].descriptorCount = 1;
    descriptorPoolSize[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descriptorPoolSize[1].descriptorCount = 1;

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo;
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.pNext = nullptr;
    descriptorPoolCreateInfo.flags = 0;
    descriptorPoolCreateInfo.maxSets = static_cast<uint32_t>(descriptorPoolSize.size());
    descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSize.size());
    descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSize.data();

    VkResult result = vkCreateDescriptorPool(m_vulkanDevice, &descriptorPoolCreateInfo, s_allocator, &m_vulkanDescriptorPool);
    if (result != VK_SUCCESS)
    {
        DUCK_DEMO_VULKAN_ASSERT(result);
        return false;
    }

    DUCK_DEMO_VULKAN_ASSERT(CreateVulkanBuffer(static_cast<VkDeviceSize>(sizeof(FrameBuf)),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, m_vulkanFrameBuffer));
    DUCK_DEMO_VULKAN_ASSERT(CreateVulkanBuffer(static_cast<VkDeviceSize>(sizeof(ObjectBuf) * 2),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, m_vulkanObjectBuffer));

    VkDescriptorSetLayoutBinding frameBufDescriptorSetLayoutBinding;
    frameBufDescriptorSetLayoutBinding.binding = 0;
    frameBufDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    frameBufDescriptorSetLayoutBinding.descriptorCount = 1;
    frameBufDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    frameBufDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding objectBufDescriptorSetLayoutBinding;
    objectBufDescriptorSetLayoutBinding.binding = 0;
    objectBufDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    objectBufDescriptorSetLayoutBinding.descriptorCount = 1;
    objectBufDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    objectBufDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo frameBufDescriptorSetLayoutCreateInfo;
    frameBufDescriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    frameBufDescriptorSetLayoutCreateInfo.pNext = nullptr;
    frameBufDescriptorSetLayoutCreateInfo.flags = 0;
    frameBufDescriptorSetLayoutCreateInfo.bindingCount = 1;
    frameBufDescriptorSetLayoutCreateInfo.pBindings = &frameBufDescriptorSetLayoutBinding;

    VkDescriptorSetLayoutCreateInfo objectBufDescriptorSetLayoutCreateInfo;
    objectBufDescriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    objectBufDescriptorSetLayoutCreateInfo.pNext = nullptr;
    objectBufDescriptorSetLayoutCreateInfo.flags = 0;
    objectBufDescriptorSetLayoutCreateInfo.bindingCount = 1;
    objectBufDescriptorSetLayoutCreateInfo.pBindings = &objectBufDescriptorSetLayoutBinding;

    result = vkCreateDescriptorSetLayout(m_vulkanDevice, &frameBufDescriptorSetLayoutCreateInfo, s_allocator, &m_vulkanDescriptorSetLayouts[0]);
    if (result != VK_SUCCESS)
    {
        DUCK_DEMO_VULKAN_ASSERT(result);
        return false;
    }

    result = vkCreateDescriptorSetLayout(m_vulkanDevice, &objectBufDescriptorSetLayoutCreateInfo, s_allocator, &m_vulkanDescriptorSetLayouts[1]);
    if (result != VK_SUCCESS)
    {
        DUCK_DEMO_VULKAN_ASSERT(result);
        return false;
    }

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.pNext = nullptr;
    descriptorSetAllocateInfo.descriptorPool = m_vulkanDescriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = static_cast<uint32_t>(m_vulkanDescriptorSetLayouts.size());
    descriptorSetAllocateInfo.pSetLayouts = m_vulkanDescriptorSetLayouts.data();

    DUCK_DEMO_VULKAN_ASSERT(vkAllocateDescriptorSets(m_vulkanDevice, &descriptorSetAllocateInfo, m_vulkanDescriptorSets.data()));

    VkDescriptorBufferInfo frameBufDescriptorBufferInfo;
    frameBufDescriptorBufferInfo.buffer = m_vulkanFrameBuffer.m_buffer;
    frameBufDescriptorBufferInfo.offset = 0;
    frameBufDescriptorBufferInfo.range = sizeof(FrameBuf);

    VkDescriptorBufferInfo objectBufDescriptorBufferInfo;
    objectBufDescriptorBufferInfo.buffer = m_vulkanObjectBuffer.m_buffer;
    objectBufDescriptorBufferInfo.offset = 0;
    objectBufDescriptorBufferInfo.range = sizeof(ObjectBuf);

    VkWriteDescriptorSet frameBufWriteDescriptorSet;
    frameBufWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    frameBufWriteDescriptorSet.pNext = nullptr;
    frameBufWriteDescriptorSet.dstSet = m_vulkanDescriptorSets[0];
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
    objectBufWriteDescriptorSet.dstSet = m_vulkanDescriptorSets[1];
    objectBufWriteDescriptorSet.dstBinding = 0;
    objectBufWriteDescriptorSet.dstArrayElement = 0;
    objectBufWriteDescriptorSet.descriptorCount = 1;
    objectBufWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    objectBufWriteDescriptorSet.pBufferInfo = &objectBufDescriptorBufferInfo;
    objectBufWriteDescriptorSet.pImageInfo = nullptr;
    objectBufWriteDescriptorSet.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(m_vulkanDevice, 1, &frameBufWriteDescriptorSet, 0, nullptr);
    vkUpdateDescriptorSets(m_vulkanDevice, 1, &objectBufWriteDescriptorSet, 0, nullptr);

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.pNext = nullptr;
    pipelineLayoutCreateInfo.flags = 0;
    pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(m_vulkanDescriptorSetLayouts.size());
    pipelineLayoutCreateInfo.pSetLayouts = m_vulkanDescriptorSetLayouts.data();
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

    std::array<VkVertexInputAttributeDescription,2> vertexInputAttributeDescriptions;
    vertexInputAttributeDescriptions[0].location = 0;
    vertexInputAttributeDescriptions[0].binding = 0;
    vertexInputAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInputAttributeDescriptions[0].offset = offsetof(MeshLoader::Vertex, position);

    vertexInputAttributeDescriptions[1].location = 1;
    vertexInputAttributeDescriptions[1].binding = 0;
    vertexInputAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInputAttributeDescriptions[1].offset = offsetof(MeshLoader::Vertex, normal);

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
    //pipelineRasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_LINE;
    pipelineRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_NONE;
    //pipelineRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
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

    {
        std::unique_ptr<DuckDemoFile> modelFile = DuckDemoUtils::LoadFileFromDisk("../kachujin_g_rosales/kachujin_g_rosales.fbx");
        if (modelFile == nullptr)
        {
            DUCK_DEMO_ASSERT(false);
            return false;
        }

        if (!MeshLoader::Loader::LoadModel(modelFile->buffer.get(), modelFile->bufferSize, m_duckMesh))
        {
            DUCK_DEMO_ASSERT(false);
            return false;
        }

        DUCK_DEMO_VULKAN_ASSERT(CreateVulkanBuffer(static_cast<VkDeviceSize>(sizeof(MeshLoader::Vertex) * m_duckMesh.vertexCount), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, m_vulkanDuckVertexBuffer));
        DUCK_DEMO_VULKAN_ASSERT(CreateVulkanBuffer(static_cast<VkDeviceSize>(sizeof(MeshLoader::IndexType) * m_duckMesh.indexCount), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, m_vulkanDuckIndexBuffer));

        FillVulkanBuffer(m_vulkanDuckVertexBuffer, m_duckMesh.GetVertex(), sizeof(MeshLoader::Vertex) * m_duckMesh.vertexCount);
        FillVulkanBuffer(m_vulkanDuckIndexBuffer, m_duckMesh.GetIndex(), sizeof(MeshLoader::IndexType) * m_duckMesh.indexCount);
    }

    {
        if (!MeshLoader::Loader::LoadCubePrimitive(m_floorMesh, 50.0f, 5.0f, 50.0f))
        {
            DUCK_DEMO_ASSERT(false);
            return false;
        }

        DUCK_DEMO_VULKAN_ASSERT(CreateVulkanBuffer(static_cast<VkDeviceSize>(sizeof(MeshLoader::Vertex) * m_floorMesh.vertexCount), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, m_vulkanFloorVertexBuffer));
        DUCK_DEMO_VULKAN_ASSERT(CreateVulkanBuffer(static_cast<VkDeviceSize>(sizeof(MeshLoader::IndexType) * m_floorMesh.indexCount), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, m_vulkanFloorIndexBuffer));

        FillVulkanBuffer(m_vulkanFloorVertexBuffer, m_floorMesh.GetVertex(), sizeof(MeshLoader::Vertex) * m_floorMesh.vertexCount);
        FillVulkanBuffer(m_vulkanFloorIndexBuffer, m_floorMesh.GetIndex(), sizeof(MeshLoader::IndexType) * m_floorMesh.indexCount);
    }

    {
        const glm::vec3 objectPosition = glm::vec3(0.0f);
        const glm::vec3 objectScale = glm::vec3(1.0f);
        const glm::quat objectRotation = glm::quat(glm::vec3(glm::radians(180.0f), 0.0f, 0.0f));

        ObjectBuf objectBuf;
        objectBuf.uWorld = glm::translate(objectPosition) * glm::toMat4(objectRotation) * glm::scale(objectScale);
        objectBuf.uWorld = glm::transpose(objectBuf.uWorld);
        objectBuf.uDiffuseAlbedo = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        objectBuf.uFresnelR0 = glm::vec3(0.02f, 0.02f, 0.02f);
        objectBuf.uRoughness = 0.2f;
        FillVulkanBuffer(m_vulkanObjectBuffer, &objectBuf, sizeof(objectBuf), sizeof(ObjectBuf) * 0);
    }

    {
        const glm::vec3 objectPosition = glm::vec3(0.0f, 20.0f, 25.0f);
        const glm::vec3 objectScale = glm::vec3(1.0f);
        const glm::quat objectRotation = glm::quat(glm::vec3(0.0f, 0.0f, 0.0f));

        ObjectBuf objectBuf;
        objectBuf.uWorld = glm::translate(objectPosition) * glm::toMat4(objectRotation) * glm::scale(objectScale);
        objectBuf.uWorld = glm::transpose(objectBuf.uWorld);
        objectBuf.uDiffuseAlbedo = glm::vec4(0.930f, 0.530f, 0.823f, 1.0f);
        objectBuf.uFresnelR0 = glm::vec3(0.02f, 0.02f, 0.02f);
        objectBuf.uRoughness = 0.2f;
        FillVulkanBuffer(m_vulkanObjectBuffer, &objectBuf, sizeof(objectBuf), sizeof(ObjectBuf) * 1);
    }

    const glm::mat4 cameraRotation = glm::toMat4(glm::quat(glm::vec3(glm::radians(0.0f), glm::radians(0.0f), 0.0f)));
    const glm::vec3 cameraPosition = glm::vec3(0.0f, -80.0f, -150.0f);

    const glm::vec3 cameraForward = cameraRotation * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

    const glm::vec3 lookAtUp = cameraRotation * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    const glm::vec3 lookAtTarget = cameraPosition + cameraForward;
    const glm::mat4x4 view = glm::lookAt(cameraPosition, lookAtTarget, lookAtUp);

    const glm::mat4x4 proj = glm::perspectiveFov(
        glm::radians(90.0f), 
        static_cast<float>(m_vulkanSwapchainWidth), 
        static_cast<float>(m_vulkanSwapchainHeight), 
        1.0f, 
        1000.0f);

    FrameBuf frameBuf;
    frameBuf.uViewProj = glm::transpose(proj * view);
    frameBuf.uEyePosW = cameraPosition;
    
    frameBuf.uAmbientLight = glm::vec4(0.25f, 0.25f, 0.25f, 1.0f);

    frameBuf.uDirLight.uDirection = glm::vec3(0.57735f, -0.57735f, 0.57735f);
    frameBuf.uDirLight.uStrength = glm::vec3(0.6f, 0.6f, 0.6f);
    FillVulkanBuffer(m_vulkanFrameBuffer, &frameBuf, sizeof(frameBuf));

    return true;
}

bool DuckDemoGame::InitFrameBuffers()
{
    m_vulkanFrameBuffers.clear();

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

        m_vulkanFrameBuffers.push_back(framebuffer);
    }

    return true;
}

void DuckDemoGame::DestroyFrameBuffers()
{
    for (VkFramebuffer frameBuffer : m_vulkanFrameBuffers)
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
    renderPassBeginInfo.framebuffer = m_vulkanFrameBuffers[m_currentSwapchainImageIndex];
    renderPassBeginInfo.renderArea.extent.width = m_vulkanSwapchainWidth;
    renderPassBeginInfo.renderArea.extent.height = m_vulkanSwapchainHeight;
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &m_vulkanClearValue;
    vkCmdBeginRenderPass(m_vulkanPrimaryCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(m_vulkanPrimaryCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vulkanPipeline);

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

    vkCmdBindDescriptorSets(m_vulkanPrimaryCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vulkanPipelineLayout, 0, 1, &m_vulkanDescriptorSets[0], 0, nullptr);

    // duck
    {
        uint32_t dynamicOffsets = sizeof(ObjectBuf) * 0;
        vkCmdBindDescriptorSets(m_vulkanPrimaryCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vulkanPipelineLayout, 1, 1, &m_vulkanDescriptorSets[1], 1, &dynamicOffsets);

        const VkDeviceSize vertexOffset = 0;
        vkCmdBindVertexBuffers(m_vulkanPrimaryCommandBuffer, 0, 1, &m_vulkanDuckVertexBuffer.m_buffer, &vertexOffset);
        vkCmdBindIndexBuffer(m_vulkanPrimaryCommandBuffer, m_vulkanDuckIndexBuffer.m_buffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(m_vulkanPrimaryCommandBuffer, m_duckMesh.indexCount, 1, 0, 0, 0);
    }

    // floor
    {
        uint32_t dynamicOffsets = sizeof(ObjectBuf) * 1;
        vkCmdBindDescriptorSets(m_vulkanPrimaryCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vulkanPipelineLayout, 1, 1, &m_vulkanDescriptorSets[1], 1, &dynamicOffsets);

        const VkDeviceSize vertexOffset = 0;
        vkCmdBindVertexBuffers(m_vulkanPrimaryCommandBuffer, 0, 1, &m_vulkanFloorVertexBuffer.m_buffer, &vertexOffset);
        vkCmdBindIndexBuffer(m_vulkanPrimaryCommandBuffer, m_vulkanFloorIndexBuffer.m_buffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(m_vulkanPrimaryCommandBuffer, m_floorMesh.indexCount, 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(m_vulkanPrimaryCommandBuffer);
}
