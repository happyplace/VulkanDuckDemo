#include "DuckDemoGame.h"

#include "DuckDemoUtils.h"

DuckDemoGame::DuckDemoGame()
{
    DUCK_DEMO_ASSERT(!ms_instance);
    ms_instance = this;
}

DuckDemoGame::~DuckDemoGame()
{
    DUCK_DEMO_ASSERT(ms_instance == this);
    ms_instance = nullptr;

    if (m_vulkanRenderPass)
    {
        vkDestroyRenderPass(m_vulkanDevice, m_vulkanRenderPass, s_allocator);
    }
}

bool DuckDemoGame::OnInit()
{
    if (!InitRenderPass())
    {
        return false;
    }

    return true;
}

bool DuckDemoGame::InitRenderPass()
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

    return true;
}

void DuckDemoGame::OnResize()
{

}

void DuckDemoGame::OnUpdate(const GameTimer& gameTimer)
{
}

void DuckDemoGame::OnRender()
{
    VkRenderPassBeginInfo renderPassBeginInfo;
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.pNext = nullptr;
    renderPassBeginInfo.renderPass = GetRenderPass();
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
