#pragma once

#include <memory>
#include <vector>
#include <string>
#include <cinttypes>

#include <SDL.h>
#include <vulkan/vulkan.h>
#include "shaderc/shaderc.h" 

#include "GameTimer.h"

struct VulkanBuffer
{
    ~VulkanBuffer();
    void Reset();

	VkBuffer m_buffer = VK_NULL_HANDLE;
	VkDeviceMemory m_deviceMemory = VK_NULL_HANDLE;
	VkDeviceSize m_deviceSize = 0;
};

class Game
{
public:
    static Game* Get();

    virtual ~Game();

    int Run(int argc, char** argv);

    SDL_Window* GetWindow() const { return m_window; }
    VkDevice GetVulkanDevice() const { return m_vulkanDevice; }

    void QuitGame();

protected:
    static Game* ms_instance;

    virtual bool OnInit() = 0;
    virtual void OnResize() = 0;
    virtual void OnUpdate(const GameTimer& gameTimer) = 0;
    virtual void OnRender() = 0;
    virtual void OnImGui() = 0;

    void RenderImGui();

    VkResult CompileShaderFromDisk(const std::string& path, const shaderc_shader_kind shaderKind, VkShaderModule* OutShaderModule, const shaderc_compile_options_t compileOptions = nullptr);
    VkResult CreateVulkanBuffer(const VkDeviceSize deviceSize, const VkBufferUsageFlagBits bufferUsageFlagBits, VulkanBuffer& OutVulkanBuffer);
    void FillVulkanBuffer(VulkanBuffer& vulkanBuffer, const void* data, const std::size_t dataSize, VkDeviceSize offset = 0);
    int32_t FindMemoryByFlagAndType(const VkMemoryPropertyFlagBits memoryFlagBits, const uint32_t memoryTypeBits) const;
    // this will take the size and make sure it will alighn with uniform buffer alightment rules of the gpu
    VkDeviceSize CalculateUniformBufferSize(const std::size_t size) const;

    VkDevice m_vulkanDevice = VK_NULL_HANDLE;
    VkFormat m_vulkanSwapchainPixelFormat = VK_FORMAT_UNDEFINED;
    VkClearValue m_vulkanClearValue;
    VkCommandBuffer m_vulkanPrimaryCommandBuffer = VK_NULL_HANDLE;
    uint32_t m_vulkanSwapchainWidth = 0;
    uint32_t m_vulkanSwapchainHeight = 0;
    uint32_t m_currentSwapchainImageIndex = 0;
    std::vector<VkImageView> m_vulkanSwapchainImageViews;
    VkImageView m_vulkanDepthStencilImageView = VK_NULL_HANDLE;

private:
    bool InitWindow();
    bool InitVulkanInstance();
    bool InitVulkanDevice();
    bool InitVulkanSwapChain(const int32_t width, const int32_t height);
    bool InitVulkanGameResources();
    bool InitVulkanDepthStencilImage();
    void FreeVulkanDepthStencilImage();

    void Update();
    void Resize(int32_t width = -1, int32_t height = -1);
    bool BeginRender();
    void EndRender();

    bool InitImGui();
    void ResizeImGui();

    bool m_quit = false;
    GameTimer m_gameTimer;

    SDL_Window* m_window = nullptr;
    VkInstance m_instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_vulkanSurface = VK_NULL_HANDLE;
    uint32_t m_vulkanGraphicsQueueIndex = 0;
    VkPhysicalDevice m_vulkanPhysicalDevice;
    VkQueue m_vulkanQueue = VK_NULL_HANDLE;
    VkSwapchainKHR m_vulkanSwapchain = VK_NULL_HANDLE;
    VkSemaphore m_vulkanAquireSwapchain = VK_NULL_HANDLE;
    VkSemaphore m_vulkanReleaseSwapchain = VK_NULL_HANDLE;
    VkCommandPool m_vulkanPrimaryCommandPool;
    VkFence m_vulkanSubmitFence = VK_NULL_HANDLE;
    shaderc_compiler_t m_shaderCompiler = nullptr;
    VkImage m_vulkanDepthStencilImage = VK_NULL_HANDLE;
    VkDeviceMemory m_vulkanDepthStencilImageMemory = VK_NULL_HANDLE;
    VkDeviceSize m_minUniformBufferOffsetAlignment = 0;
    uint32_t m_vulkanSwapChainImageCount = 0;
    VkDescriptorPool m_imguiDescriptorPool = VK_NULL_HANDLE;
    VkRenderPass m_imguiRenderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> m_imguiFrameBuffers;

#ifdef DUCK_DEMO_VULKAN_DEBUG
    VkDebugReportCallbackEXT m_debugReportCallbackExt = VK_NULL_HANDLE;
#endif // DUCK_DEMO_VULKAN_DEBUG
};
