#pragma once

#include <vector>

#include <vulkan/vulkan.h>

typedef union SDL_Event SDL_Event;

struct ImGuiRenderPass
{
    VkDescriptorPool m_imguiDescriptorPool = VK_NULL_HANDLE;
    VkRenderPass m_imguiRenderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> m_imguiFrameBuffers;
};

bool Init_ImGuiRenderPass(ImGuiRenderPass& imguiRenderPass);
void Free_ImGuiRenderPass(ImGuiRenderPass& imguiRenderPass);

void ProcessEvent_ImGuiRenderPass(const SDL_Event* sdlEvent);
void Resize_ImGuiRenderPass(ImGuiRenderPass& imguiRenderPass);
void BeginRender_ImGuiRenderPass(ImGuiRenderPass& imguiRenderPass);
void EndRender_ImGuiRenderPass(ImGuiRenderPass& imguiRenderPass, VkCommandBuffer commandBuffer);
