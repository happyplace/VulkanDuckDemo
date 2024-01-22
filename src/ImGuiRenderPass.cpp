#include "ImGuiRenderPass.h"

#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"

#include <SDL_events.h>

#include "DuckDemoUtils.h"
#include "Game.h"

void ImGuiInitAssert(VkResult result)
{
    DUCK_DEMO_VULKAN_ASSERT(result);
}

bool Init_ImGuiRenderPass(ImGuiRenderPass& imguiRenderPass)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    ImGui_ImplSDL2_InitForVulkan(Game::Get()->GetWindow());

    VkDescriptorPoolSize pool_sizes[] =
    {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
    };
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1;
    pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;
    VkResult result = vkCreateDescriptorPool(Game::Get()->GetVulkanDevice(), &pool_info, s_allocator, &imguiRenderPass.m_imguiDescriptorPool);
    if (result != VK_SUCCESS)
    {
        DUCK_DEMO_VULKAN_ASSERT(result);
        return false;
    }

    {
        VkAttachmentDescription attachment = {};
        attachment.format = Game::Get()->GetVulkanSwapchainPixelFormat();
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        VkAttachmentReference color_attachment = {};
        color_attachment.attachment = 0;
        color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_attachment;
        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        VkRenderPassCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        info.attachmentCount = 1;
        info.pAttachments = &attachment;
        info.subpassCount = 1;
        info.pSubpasses = &subpass;
        info.dependencyCount = 1;
        info.pDependencies = &dependency;
        result = vkCreateRenderPass(Game::Get()->GetVulkanDevice(), &info, s_allocator, &imguiRenderPass.m_imguiRenderPass);
        if (result != VK_SUCCESS)
        {
            DUCK_DEMO_VULKAN_ASSERT(result);
            return false;
        }
    }

    ImGui_ImplVulkan_InitInfo initInfo;
    memset(&initInfo, 0, sizeof(initInfo));
    initInfo.Instance = Game::Get()->GetVulkanInstance();
    initInfo.PhysicalDevice = Game::Get()->GetVulkanPhysicalDevice();
    initInfo.Device = Game::Get()->GetVulkanDevice();
    initInfo.QueueFamily = Game::Get()->GetVulkanGraphicsQueueIndex();
    initInfo.Queue = Game::Get()->GetVulkanQueue();
    initInfo.PipelineCache = VK_NULL_HANDLE;
    initInfo.DescriptorPool = imguiRenderPass.m_imguiDescriptorPool;
    initInfo.Subpass = 0;
    initInfo.MinImageCount = Game::Get()->GetVulkanSwapChainImageCount();
    initInfo.ImageCount = Game::Get()->GetVulkanSwapChainImageCount();
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initInfo.Allocator = s_allocator;
    initInfo.CheckVkResultFn = ImGuiInitAssert;
    initInfo.UseDynamicRendering = false;
    if (!ImGui_ImplVulkan_Init(&initInfo, imguiRenderPass.m_imguiRenderPass))
    {
        DUCK_DEMO_ASSERT(false);
        return false;
    }

    ImGui_ImplVulkan_SetMinImageCount(Game::Get()->GetVulkanSwapChainImageCount());

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    return true;
}

void Free_ImGuiRenderPass(ImGuiRenderPass& imguiRenderPass)
{
    for (VkFramebuffer frameBuffer : imguiRenderPass.m_imguiFrameBuffers)
    {
        vkDestroyFramebuffer(Game::Get()->GetVulkanDevice(), frameBuffer, s_allocator);
    }

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    if (imguiRenderPass.m_imguiRenderPass)
    {
        vkDestroyRenderPass(Game::Get()->GetVulkanDevice(), imguiRenderPass.m_imguiRenderPass, s_allocator);
    }

    if (imguiRenderPass.m_imguiDescriptorPool)
    {
        vkDestroyDescriptorPool(Game::Get()->GetVulkanDevice(), imguiRenderPass.m_imguiDescriptorPool, s_allocator);
    }
}

void ProcessEvent_ImGuiRenderPass(const SDL_Event* sdlEvent)
{
    ImGui_ImplSDL2_ProcessEvent(sdlEvent);
}

void Resize_ImGuiRenderPass(ImGuiRenderPass& imguiRenderPass)
{
    for (VkFramebuffer frameBuffer : imguiRenderPass.m_imguiFrameBuffers)
    {
        vkDestroyFramebuffer(Game::Get()->GetVulkanDevice(), frameBuffer, s_allocator);
    }
    
    imguiRenderPass.m_imguiFrameBuffers.clear();

    {
        VkImageView attachment[1];
        VkFramebufferCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass = imguiRenderPass.m_imguiRenderPass;
        info.attachmentCount = 1;
        info.pAttachments = attachment;
        info.width = Game::Get()->GetVulkanSwapchainWidth();
        info.height = Game::Get()->GetVulkanSwapchainHeight();
        info.layers = 1;
        for (uint32_t i = 0; i < Game::Get()->GetVulkanSwapChainImageCount(); i++)
        {
            attachment[0] = Game::Get()->GetVulkanSwapchainImageViews()[i];

            VkFramebuffer framebuffer;
            DUCK_DEMO_VULKAN_ASSERT(vkCreateFramebuffer(Game::Get()->GetVulkanDevice(), &info, s_allocator, &framebuffer));

            imguiRenderPass.m_imguiFrameBuffers.push_back(framebuffer);
        }
    }
}

void BeginRender_ImGuiRenderPass(ImGuiRenderPass& /*imguiRenderPass*/)
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void EndRender_ImGuiRenderPass(ImGuiRenderPass& imguiRenderPass, VkCommandBuffer commandBuffer)
{
    ImGui::Render();
    ImDrawData* drawData = ImGui::GetDrawData();

    {
        VkRenderPassBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = imguiRenderPass.m_imguiRenderPass;
        info.framebuffer = imguiRenderPass.m_imguiFrameBuffers[Game::Get()->GetCurrentSwapchainImageIndex()];
        info.renderArea.extent.width = Game::Get()->GetVulkanSwapchainWidth();
        info.renderArea.extent.height = Game::Get()->GetVulkanSwapchainHeight();
        info.clearValueCount = 0;
        info.pClearValues = nullptr;
        vkCmdBeginRenderPass(commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);

    vkCmdEndRenderPass(commandBuffer);
}
