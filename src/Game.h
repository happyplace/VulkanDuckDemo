#pragma once

#include <memory>
#include <vector>
#include <string>
#include <cinttypes>

#include <SDL.h>
#include <vulkan/vulkan.h>
#include "shaderc/shaderc.h" 

#include "GameTimer.h"
#include "ImGuiRenderPass.h"
#include "VulkanBuffer.h"
#include "VulkanTexture.h"

class Game
{
public:
    static Game* Get();

    Game();
    virtual ~Game();

    int Run(int argc, char** argv);

    VkResult CompileShaderFromDisk(const std::string& path, const shaderc_shader_kind shaderKind, VkShaderModule* OutShaderModule, const shaderc_compile_options_t compileOptions = nullptr);
    VkResult CreateVulkanBuffer(const VkDeviceSize deviceSize, const VkBufferUsageFlagBits bufferUsageFlagBits, VulkanBuffer& OutVulkanBuffer);
    void FillVulkanBuffer(VulkanBuffer& vulkanBuffer, const void* data, const std::size_t dataSize, const VkDeviceSize offset = 0);
    void ZeroVulkanBuffer(VulkanBuffer& vulkanBuffer);
    VkDeviceSize CalculateUniformBufferSize(const std::size_t size) const;
    // this will take the size and make sure it will align with uniform buffer alightment rules of the gpu
    VkResult CreateVulkanTexture(const std::string path, VulkanTexture& outVulkanTexture);
    int32_t FindMemoryByFlagAndType(const VkMemoryPropertyFlagBits memoryFlagBits, const uint32_t memoryTypeBits) const;
    void TransferFromStagingBufferToImage(VkBuffer stagingBuffer, VkImage dstImage, const uint32_t mipLevels, const uint32_t width, const uint32_t height) const;

    SDL_Window* GetWindow() const { return m_window; }
    VkDevice GetVulkanDevice() const { return m_vulkanDevice; }
    uint32_t GetVulkanSwapchainWidth() const { return m_vulkanSwapchainWidth; }
    uint32_t GetVulkanSwapchainHeight() const { return m_vulkanSwapchainHeight; }
    uint32_t GetVulkanSwapChainImageCount() const { return m_vulkanSwapChainImageCount; }
    const std::vector<VkImageView>& GetVulkanSwapchainImageViews() const { return m_vulkanSwapchainImageViews; }
    VkInstance GetVulkanInstance() const { return m_instance; }
    VkFormat GetVulkanSwapchainPixelFormat() const { return m_vulkanSwapchainPixelFormat; }
    VkPhysicalDevice GetVulkanPhysicalDevice() const { return m_vulkanPhysicalDevice; }
    uint32_t GetVulkanGraphicsQueueIndex() const { return m_vulkanGraphicsQueueIndex; }
    uint32_t GetVulkanComputeQueueIndex() const { return m_vulkanComputeQueueIndex; }
    VkQueue GetVulkanQueue() const { return m_vulkanQueue; }
    uint32_t GetCurrentSwapchainImageIndex() const { return m_currentSwapchainImageIndex; }
    VkImageView GetVulkanDepthStencilImageView() const { return m_vulkanDepthStencilImageView; }
    VkClearValue GetVulkanClearValue() const { return m_vulkanClearValue; }

    void QuitGame();

protected:
    static Game* ms_instance;

    virtual bool OnInit() = 0;
    virtual void OnResize() = 0;
    virtual void OnUpdate(const GameTimer& gameTimer) = 0;
    virtual void OnRender() = 0;

    VkDevice m_vulkanDevice = VK_NULL_HANDLE;
    VkFormat m_vulkanSwapchainPixelFormat = VK_FORMAT_UNDEFINED;
    VkClearValue m_vulkanClearValue;
    VkCommandBuffer m_vulkanPrimaryCommandBuffer = VK_NULL_HANDLE;
    uint32_t m_vulkanSwapchainWidth = 0;
    uint32_t m_vulkanSwapchainHeight = 0;
    uint32_t m_currentSwapchainImageIndex = 0;
    std::vector<VkImageView> m_vulkanSwapchainImageViews;
    VkImageView m_vulkanDepthStencilImageView = VK_NULL_HANDLE;
    VkCommandBuffer m_vulkanTempCommandBuffer = VK_NULL_HANDLE;
    VkCommandPool m_vulkanTempCommandPool = VK_NULL_HANDLE;
    VkFence m_vulkanTempFence = VK_NULL_HANDLE;
    VkPhysicalDevice m_vulkanPhysicalDevice = VK_NULL_HANDLE;
    ImGuiRenderPass m_imGuiRenderPass;

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

    bool m_quit = false;
    GameTimer m_gameTimer;

    SDL_Window* m_window = nullptr;
    VkInstance m_instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_vulkanSurface = VK_NULL_HANDLE;
    uint32_t m_vulkanGraphicsQueueIndex = 0;
    uint32_t m_vulkanComputeQueueIndex = 0;
    VkQueue m_vulkanQueue = VK_NULL_HANDLE;
    VkSwapchainKHR m_vulkanSwapchain = VK_NULL_HANDLE;
    VkSemaphore m_vulkanAquireSwapchain = VK_NULL_HANDLE;
    VkSemaphore m_vulkanReleaseSwapchain = VK_NULL_HANDLE;
    VkCommandPool m_vulkanPrimaryCommandPool = VK_NULL_HANDLE;
    VkFence m_vulkanSubmitFence = VK_NULL_HANDLE;
    shaderc_compiler_t m_shaderCompiler = nullptr;
    VkImage m_vulkanDepthStencilImage = VK_NULL_HANDLE;
    VkDeviceMemory m_vulkanDepthStencilImageMemory = VK_NULL_HANDLE;
    VkDeviceSize m_minUniformBufferOffsetAlignment = 0;
    uint32_t m_vulkanSwapChainImageCount = 0;

#ifdef DUCK_DEMO_VULKAN_DEBUG
    VkDebugReportCallbackEXT m_debugReportCallbackExt = VK_NULL_HANDLE;
#endif // DUCK_DEMO_VULKAN_DEBUG
};
