#include "Game.h"

#include <SDL_vulkan.h>

#include "DuckDemoUtils.h"

Game* Game::ms_instance = nullptr;
Game* Game::Get()
{
    DUCK_DEMO_ASSERT(ms_instance);
    return ms_instance;
}

VulkanBuffer::~VulkanBuffer()
{
    Reset();
}

void VulkanBuffer::Reset()
{
    if (m_buffer == VK_NULL_HANDLE && m_deviceMemory == VK_NULL_HANDLE)
    {
        return;
    }

    m_deviceSize = 0;

    Game* game = Game::Get();
    if (game == nullptr)
    {
        DUCK_DEMO_ASSERT(game);
        return;
    }
    
    VkDevice vulkanDevice = game->GetVulkanDevice();
    if (vulkanDevice == VK_NULL_HANDLE)
    {
        DUCK_DEMO_ASSERT(vulkanDevice != VK_NULL_HANDLE);
        return;
    }

    if (m_buffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(vulkanDevice, m_buffer, s_allocator);
        m_buffer = VK_NULL_HANDLE;
    }

    if (m_deviceMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(vulkanDevice, m_deviceMemory, s_allocator);
        m_deviceMemory = VK_NULL_HANDLE;
    }
}

Game::~Game()
{
    vkDeviceWaitIdle(m_vulkanDevice);

#ifdef DUCK_DEMO_VULKAN_DEBUG
    if (m_debugReportCallbackExt)
    {
        PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(m_instance, "vkDestroyDebugReportCallbackEXT");
        DUCK_DEMO_ASSERT(vkDestroyDebugReportCallbackEXT);

        vkDestroyDebugReportCallbackEXT(m_instance, m_debugReportCallbackExt, s_allocator);
    }
#endif // DUCK_DEMO_VULKAN_DEBUG

    shaderc_compiler_release(m_shaderCompiler);

    if (m_vulkanPrimaryCommandBuffer)
    {
        vkFreeCommandBuffers(m_vulkanDevice, m_vulkanPrimaryCommandPool, 1, &m_vulkanPrimaryCommandBuffer);
    }

    if (m_vulkanPrimaryCommandPool)
    {
        vkDestroyCommandPool(m_vulkanDevice, m_vulkanPrimaryCommandPool, s_allocator);
    }

    if (m_vulkanSubmitFence)
    {
        vkDestroyFence(m_vulkanDevice, m_vulkanSubmitFence, s_allocator);
    }

    if (m_vulkanAquireSwapchain)
    {
        vkDestroySemaphore(m_vulkanDevice, m_vulkanAquireSwapchain, s_allocator);
    }

    if (m_vulkanReleaseSwapchain)
    {
        vkDestroySemaphore(m_vulkanDevice, m_vulkanReleaseSwapchain, s_allocator);
    }

    for (VkImageView imageView : m_vulkanSwapchainImageViews)
    {
        vkDestroyImageView(m_vulkanDevice, imageView, nullptr);
    }

    if (m_vulkanSwapchain)
    {
        vkDestroySwapchainKHR(m_vulkanDevice, m_vulkanSwapchain, nullptr);
    }

    if (m_vulkanSurface)
    {
        vkDestroySurfaceKHR(m_instance, m_vulkanSurface, s_allocator);
    }

    if (m_vulkanDevice)
    {
        vkDestroyDevice(m_vulkanDevice, s_allocator);
    }

    if (m_instance)
    {
        vkDestroyInstance(m_instance, s_allocator);
    }

    if (m_window)
    {
        SDL_DestroyWindow(m_window);
    }

    SDL_Quit();
}

int Game::Run(int argc, char** argv)
{
    if (!InitWindow())
    {
        return 1;
    }

    int32_t windowWidth;
    int32_t windowHeight;
    SDL_GetWindowSize(m_window, &windowWidth, &windowHeight);

    if (!InitVulkanInstance())
    {
        return 1;
    }

    if (!InitVulkanDevice())
    {
        return 1;
    }

    if (!InitVulkanSwapChain(windowWidth, windowHeight))
    {
        return 1;
    }

    if (!InitVulkanGameResources())
    {
        return 1;
    }

    m_shaderCompiler = shaderc_compiler_initialize();
    if (m_shaderCompiler == nullptr)
    {
        return 1;
    }

    if (!OnInit())
    {
        return 1;
    }

    Resize(windowWidth, windowHeight);

    m_gameTimer.Reset();
    while (!m_quit)
    {
        SDL_Event sdlEvent;
        while (SDL_PollEvent(&sdlEvent) != 0)
        {
            if (sdlEvent.type == SDL_QUIT)
            {
                m_quit = true;
                break;
            }
            else if (sdlEvent.type == SDL_WINDOWEVENT)
            {
                if (sdlEvent.window.event == SDL_WINDOWEVENT_RESIZED)
                {
                    const Sint32 width = sdlEvent.window.data1;
                    const Sint32 height = sdlEvent.window.data2;
                    Resize(width, height);
                    break;
                }
            }
        }

        m_gameTimer.Tick();

        Update();
        if (!BeginRender())
        {
            continue;
        }
        OnRender();
        EndRender();
    }

    return 0;
}

void Game::Update()
{
    OnUpdate(m_gameTimer);
}

void Game::Resize(int32_t width /*= -1*/, int32_t height /*= -1*/)
{
    if (width < 0 || height < 0)
    {
        SDL_GetWindowSize(m_window, &width, &height);
    }

    DUCK_DEMO_ASSERT(m_vulkanPhysicalDevice != VK_NULL_HANDLE);

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    DUCK_DEMO_VULKAN_ASSERT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_vulkanPhysicalDevice, m_vulkanSurface, &surfaceCapabilities));

    vkDeviceWaitIdle(m_vulkanDevice);

    InitVulkanSwapChain(width, height);

    OnResize();
}

void Game::QuitGame()
{
    m_quit = true;
}

bool Game::InitWindow()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0)
    {
        DUCK_DEMO_SHOW_ERROR("Window Initialization Fail", DuckDemoUtils::format("SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER)\n%s", SDL_GetError()));
        return false;
    }

    m_window = SDL_CreateWindow("Vulkan Duck Demo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (m_window == nullptr)
    {
        DUCK_DEMO_SHOW_ERROR("Window Initialization Fail", DuckDemoUtils::format("SDL_CreateWindow\n%s", SDL_GetError()));
        return false;
    }

    return true;
}

#ifdef DUCK_DEMO_VULKAN_DEBUG
/// @brief A debug callback called from Vulkan validation layers.
static VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(
    VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT type, uint64_t object, size_t location, int32_t message_code, 
    const char *layer_prefix, const char *message, void *user_data)
{
    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Validation Layer: %s: %s", layer_prefix, message);
    }
    else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Validation Layer: %s: %s", layer_prefix, message);
    }
    else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Performance warning: %s: %s", layer_prefix, message);
    }
    else
    {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Validation Layer: Information: %s: %s", layer_prefix, message);
    }
    return VK_FALSE;
}
#endif // DUCK_DEMO_VULKAN_DEBUG

bool Game::InitVulkanInstance()
{
    uint32_t extensionCount;
    if (!SDL_Vulkan_GetInstanceExtensions(m_window, &extensionCount, nullptr))
    {
        DUCK_DEMO_SHOW_ERROR("Vulkan Initialization Fail", "Failed to get instance extension count");
        return false;
    }

    std::vector<const char*> activeExtensionNames(extensionCount);
    if (!SDL_Vulkan_GetInstanceExtensions(m_window, &extensionCount, activeExtensionNames.data()))
    {
        DUCK_DEMO_SHOW_ERROR("Vulkan Initialization Fail", "Failed to get instance extension names");
        return false;
    }

    uint32_t instanceExtensionCount;
    DUCK_DEMO_VULKAN_ASSERT(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr));

    std::vector<VkExtensionProperties> instanceExtensions(instanceExtensionCount);
    DUCK_DEMO_VULKAN_ASSERT(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, instanceExtensions.data()));

    uint32_t instanceLayerCount;
    DUCK_DEMO_VULKAN_ASSERT(vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr));

    std::vector<VkLayerProperties> supportedValidationLayers(instanceLayerCount);
    DUCK_DEMO_VULKAN_ASSERT(vkEnumerateInstanceLayerProperties(&instanceLayerCount, supportedValidationLayers.data()));

    VkApplicationInfo applicationInfo;
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pNext = nullptr;
    applicationInfo.pApplicationName = "Vulkan Duck Demo";
    applicationInfo.pEngineName = "Vulkan Duck Demo Engine";
    applicationInfo.applicationVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
    applicationInfo.engineVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
    applicationInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);

    std::vector<const char*> layerNames;
#ifdef DUCK_DEMO_VULKAN_DEBUG
    layerNames.push_back("VK_LAYER_KHRONOS_validation");
#endif // DUCK_DEMO_VULKAN_DEBUG

    VkInstanceCreateInfo instanceInfo;
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pNext = nullptr;
    instanceInfo.flags = 0;
    instanceInfo.pApplicationInfo = &applicationInfo;
    instanceInfo.enabledLayerCount = static_cast<uint32_t>(layerNames.size());
    instanceInfo.ppEnabledLayerNames = layerNames.data();

#ifdef DUCK_DEMO_VULKAN_DEBUG
    VkDebugReportCallbackCreateInfoEXT debugReportCreateInfo;
    debugReportCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    debugReportCreateInfo.pNext = nullptr;
    debugReportCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    debugReportCreateInfo.pfnCallback = vulkan_debug_callback;
    debugReportCreateInfo.pUserData = nullptr;

    instanceInfo.pNext = &debugReportCreateInfo;
#endif // DUCK_DEMO_VULKAN_DEBUG

#ifdef DUCK_DEMO_VULKAN_PORTABILITY
    instanceInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

    activeExtensionNames.push_back("VK_KHR_portability_enumeration");
    activeExtensionNames.push_back("VK_KHR_get_physical_device_properties2");
#endif // DUCK_DEMO_VULKAN_PORTABILITY

#ifdef DUCK_DEMO_VULKAN_DEBUG
    activeExtensionNames.push_back("VK_EXT_debug_report");
#endif // DUCK_DEMO_VULKAN_DEBUG

    // check that requested extensions are actually supported
    for (uint32_t i = 0; i < activeExtensionNames.size(); ++i)
    {
        bool requestedExtensionAvailable = false;
        for (const VkExtensionProperties& extensionProperties : instanceExtensions)
        {
            if (strcmp(activeExtensionNames[i], extensionProperties.extensionName) == 0)
            {
                requestedExtensionAvailable = true;
                break;
            }
        }
        DUCK_DEMO_ASSERT(requestedExtensionAvailable);
    }

    instanceInfo.enabledExtensionCount = static_cast<uint32_t>(activeExtensionNames.size());
    instanceInfo.ppEnabledExtensionNames = activeExtensionNames.data();

    DUCK_DEMO_VULKAN_ASSERT(vkCreateInstance(&instanceInfo, s_allocator, &m_instance));

    if (!SDL_Vulkan_CreateSurface(m_window, m_instance, &m_vulkanSurface))
    {
        DUCK_DEMO_SHOW_ERROR("Vulkan Initialization Fail", "Failed to create a surface");
        return false;
    }

#ifdef DUCK_DEMO_VULKAN_DEBUG
    PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(m_instance, "vkCreateDebugReportCallbackEXT");
    DUCK_DEMO_ASSERT(vkCreateDebugReportCallbackEXT);

    DUCK_DEMO_VULKAN_ASSERT(vkCreateDebugReportCallbackEXT(m_instance, &debugReportCreateInfo, s_allocator, &m_debugReportCallbackExt));
#endif // DUCK_DEMO_VULKAN_DEBUG

    return true;
}

bool Game::InitVulkanDevice()
{
    uint32_t gpuCount = 0;
    DUCK_DEMO_VULKAN_ASSERT(vkEnumeratePhysicalDevices(m_instance, &gpuCount, nullptr));

    std::vector<VkPhysicalDevice> gpus(gpuCount);
    DUCK_DEMO_VULKAN_ASSERT(vkEnumeratePhysicalDevices(m_instance, &gpuCount, gpus.data()));

    int32_t graphicsQueueIndex = -1;
    int8_t deviceTypeScore = -1;

    for (uint32_t i = 0; i < gpuCount; ++i)
    {
        uint32_t queueFamilyPropertyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(gpus[i], &queueFamilyPropertyCount, nullptr);

        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(gpus[i], &deviceProperties);

        std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(gpus[i], &queueFamilyPropertyCount, queueFamilyProperties.data());

        for (uint32_t k = 0; k < queueFamilyPropertyCount; ++k)
        {
            VkQueueFamilyProperties familyProperty = queueFamilyProperties[k];

            VkBool32 supportsPresent = false;
            DUCK_DEMO_VULKAN_ASSERT(vkGetPhysicalDeviceSurfaceSupportKHR(gpus[i], k, m_vulkanSurface, &supportsPresent));

            if ((familyProperty.queueFlags & VK_QUEUE_GRAPHICS_BIT) && supportsPresent)
            {
                if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceTypeScore < VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                {
                    m_vulkanPhysicalDevice = gpus[i];
                    graphicsQueueIndex = k;
                    deviceTypeScore = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
                    break;
                }
                else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU && deviceTypeScore < VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
                {
                    m_vulkanPhysicalDevice = gpus[i];
                    graphicsQueueIndex = k;
                    deviceTypeScore = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
                }
            }
        }
    }

    if (graphicsQueueIndex <= -1)
    {  
        DUCK_DEMO_SHOW_ERROR("Vulkan Initialization Fail", "Did not find suitable queue which supports graphics, compute and presentation.");
        return false;
    }

    m_vulkanGraphicsQueueIndex = static_cast<uint32_t>(graphicsQueueIndex);

    uint32_t deviceExtensionCount;
    DUCK_DEMO_VULKAN_ASSERT(vkEnumerateDeviceExtensionProperties(m_vulkanPhysicalDevice, nullptr, &deviceExtensionCount, nullptr));

    std::vector<VkExtensionProperties> deviceExtensions(deviceExtensionCount);
    DUCK_DEMO_VULKAN_ASSERT(vkEnumerateDeviceExtensionProperties(m_vulkanPhysicalDevice, nullptr, &deviceExtensionCount, deviceExtensions.data()));

    std::vector<const char*> requiredExtensionNames;
    requiredExtensionNames.push_back("VK_KHR_swapchain");

    // check that requested device
    for (uint32_t i = 0; i < requiredExtensionNames.size(); ++i)
    {
        bool requestedExtensionAvailableOnDevice = false;
        for (const VkExtensionProperties& extensionProperties : deviceExtensions)
        {
            if (strcmp(requiredExtensionNames[i], extensionProperties.extensionName) == 0)
            {
                requestedExtensionAvailableOnDevice = true;
                break;
            }
        }
        DUCK_DEMO_ASSERT(requestedExtensionAvailableOnDevice);
    }

    VkDeviceQueueCreateInfo deviceQueueCreateInfo;
    deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceQueueCreateInfo.pNext = nullptr;
    deviceQueueCreateInfo.flags = 0;
    deviceQueueCreateInfo.queueCount = 1;
    float queue_priority = 1.0f;
    deviceQueueCreateInfo.pQueuePriorities = &queue_priority;
    deviceQueueCreateInfo.queueFamilyIndex = m_vulkanGraphicsQueueIndex;

    VkPhysicalDeviceFeatures physicalDeviceFeatures;
    physicalDeviceFeatures.robustBufferAccess = VK_FALSE;
    physicalDeviceFeatures.fullDrawIndexUint32 = VK_FALSE;
    physicalDeviceFeatures.imageCubeArray = VK_FALSE;
    physicalDeviceFeatures.independentBlend = VK_FALSE;
    physicalDeviceFeatures.geometryShader = VK_FALSE;
    physicalDeviceFeatures.tessellationShader = VK_FALSE;
    physicalDeviceFeatures.sampleRateShading = VK_FALSE;
    physicalDeviceFeatures.dualSrcBlend = VK_FALSE;
    physicalDeviceFeatures.logicOp = VK_FALSE;
    physicalDeviceFeatures.multiDrawIndirect = VK_FALSE;
    physicalDeviceFeatures.drawIndirectFirstInstance = VK_FALSE;
    physicalDeviceFeatures.depthClamp = VK_FALSE;
    physicalDeviceFeatures.depthBiasClamp = VK_FALSE;
    physicalDeviceFeatures.fillModeNonSolid = VK_TRUE;
    physicalDeviceFeatures.depthBounds = VK_FALSE;
    physicalDeviceFeatures.wideLines = VK_FALSE;
    physicalDeviceFeatures.largePoints = VK_FALSE;
    physicalDeviceFeatures.alphaToOne = VK_FALSE;
    physicalDeviceFeatures.multiViewport = VK_FALSE;
    physicalDeviceFeatures.samplerAnisotropy = VK_FALSE;
    physicalDeviceFeatures.textureCompressionETC2 = VK_FALSE;
    physicalDeviceFeatures.textureCompressionASTC_LDR = VK_FALSE;
    physicalDeviceFeatures.textureCompressionBC = VK_FALSE;
    physicalDeviceFeatures.occlusionQueryPrecise = VK_FALSE;
    physicalDeviceFeatures.pipelineStatisticsQuery = VK_FALSE;
    physicalDeviceFeatures.vertexPipelineStoresAndAtomics = VK_FALSE;
    physicalDeviceFeatures.fragmentStoresAndAtomics = VK_FALSE;
    physicalDeviceFeatures.shaderTessellationAndGeometryPointSize = VK_FALSE;
    physicalDeviceFeatures.shaderImageGatherExtended = VK_FALSE;
    physicalDeviceFeatures.shaderStorageImageExtendedFormats = VK_FALSE;
    physicalDeviceFeatures.shaderStorageImageMultisample = VK_FALSE;
    physicalDeviceFeatures.shaderStorageImageReadWithoutFormat = VK_FALSE;
    physicalDeviceFeatures.shaderStorageImageWriteWithoutFormat = VK_FALSE;
    physicalDeviceFeatures.shaderUniformBufferArrayDynamicIndexing = VK_FALSE;
    physicalDeviceFeatures.shaderSampledImageArrayDynamicIndexing = VK_FALSE;
    physicalDeviceFeatures.shaderStorageBufferArrayDynamicIndexing = VK_FALSE;
    physicalDeviceFeatures.shaderStorageImageArrayDynamicIndexing = VK_FALSE;
    physicalDeviceFeatures.shaderClipDistance = VK_FALSE;
    physicalDeviceFeatures.shaderCullDistance = VK_FALSE;
    physicalDeviceFeatures.shaderFloat64 = VK_FALSE;
    physicalDeviceFeatures.shaderInt64 = VK_FALSE;
    physicalDeviceFeatures.shaderInt16 = VK_FALSE;
    physicalDeviceFeatures.shaderResourceResidency = VK_FALSE;
    physicalDeviceFeatures.shaderResourceMinLod = VK_FALSE;
    physicalDeviceFeatures.sparseBinding = VK_FALSE;
    physicalDeviceFeatures.sparseResidencyBuffer = VK_FALSE;
    physicalDeviceFeatures.sparseResidencyImage2D = VK_FALSE;
    physicalDeviceFeatures.sparseResidencyImage3D = VK_FALSE;
    physicalDeviceFeatures.sparseResidency2Samples= VK_FALSE;
    physicalDeviceFeatures.sparseResidency4Samples = VK_FALSE;
    physicalDeviceFeatures.sparseResidency8Samples = VK_FALSE;
    physicalDeviceFeatures.sparseResidency16Samples = VK_FALSE;
    physicalDeviceFeatures.sparseResidencyAliased =VK_FALSE;
    physicalDeviceFeatures.variableMultisampleRate = VK_FALSE;
    physicalDeviceFeatures.inheritedQueries = VK_FALSE;

    VkDeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = nullptr;
    deviceCreateInfo.flags = 0;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
    deviceCreateInfo.enabledLayerCount = 0;
    deviceCreateInfo.ppEnabledLayerNames = nullptr;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensionNames.size());
    deviceCreateInfo.ppEnabledExtensionNames = requiredExtensionNames.data();
    deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;

    DUCK_DEMO_VULKAN_ASSERT(vkCreateDevice(m_vulkanPhysicalDevice, &deviceCreateInfo, s_allocator, &m_vulkanDevice));
    vkGetDeviceQueue(m_vulkanDevice, m_vulkanGraphicsQueueIndex, 0, &m_vulkanQueue);

    return true;
}

bool Game::InitVulkanSwapChain(const int32_t width, const int32_t height)
{
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    DUCK_DEMO_VULKAN_ASSERT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_vulkanPhysicalDevice, m_vulkanSurface, &surfaceCapabilities));

    uint32_t surfaceFormatCount;
    DUCK_DEMO_VULKAN_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(m_vulkanPhysicalDevice, m_vulkanSurface, &surfaceFormatCount, nullptr));
    if (surfaceFormatCount <= 0)
    {
        DUCK_DEMO_ASSERT(surfaceFormatCount > 0);
        return false;
    }
    std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
    DUCK_DEMO_VULKAN_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(m_vulkanPhysicalDevice, m_vulkanSurface, &surfaceFormatCount, surfaceFormats.data()));

    std::vector<VkFormat> preferredSurfaceFormats;
    preferredSurfaceFormats.push_back(VK_FORMAT_R8G8B8A8_SRGB);
    preferredSurfaceFormats.push_back(VK_FORMAT_B8G8R8A8_SRGB);
    preferredSurfaceFormats.push_back(VK_FORMAT_A8B8G8R8_SRGB_PACK32);

    VkSurfaceFormatKHR selectedSurfaceFormat;
    selectedSurfaceFormat.format = VK_FORMAT_UNDEFINED;

    for (VkSurfaceFormatKHR surfaceFormat : surfaceFormats)
    {
        if (selectedSurfaceFormat.format != VK_FORMAT_UNDEFINED)
        {
            break;
        }

        for (VkFormat preferredFormat : preferredSurfaceFormats)
        {
            if (surfaceFormat.format == preferredFormat)
            {
                selectedSurfaceFormat = surfaceFormat;
                break;
            }
        } 
    }

    if (selectedSurfaceFormat.format == VK_FORMAT_UNDEFINED)
    {
        selectedSurfaceFormat = surfaceFormats[0];
    }

    VkExtent2D swapChainSize;
    if (surfaceCapabilities.currentExtent.width == 0xffffffff)
    {
        swapChainSize.width = width;
        swapChainSize.height = height;
    } 
    else
    {
        swapChainSize = surfaceCapabilities.currentExtent;
    }

    VkPresentModeKHR swapChainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

    uint32_t swapChainImageCount = std::max(1u, surfaceCapabilities.minImageCount);

    VkSurfaceTransformFlagBitsKHR preTransform;
    if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
    {
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else
    {
        preTransform = surfaceCapabilities.currentTransform;
    }

    VkSwapchainKHR oldSwapchain = m_vulkanSwapchain;

    VkCompositeAlphaFlagBitsKHR compositeAlphaFlagBits = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
    {
        compositeAlphaFlagBits = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    }
    else if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
    {
        compositeAlphaFlagBits = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    }
    else if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
    {
        compositeAlphaFlagBits = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
    }
    else if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
    {
        compositeAlphaFlagBits = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
    }

    VkSwapchainCreateInfoKHR swapchainCreateInfo;
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.pNext = nullptr;
    swapchainCreateInfo.flags = 0;
    swapchainCreateInfo.surface = m_vulkanSurface;
    swapchainCreateInfo.minImageCount = swapChainImageCount;
    swapchainCreateInfo.imageFormat = selectedSurfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = selectedSurfaceFormat.colorSpace;
    swapchainCreateInfo.imageExtent.width = swapChainSize.width;
    swapchainCreateInfo.imageExtent.height = swapChainSize.height;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.queueFamilyIndexCount = 0;
    swapchainCreateInfo.pQueueFamilyIndices = nullptr;
    swapchainCreateInfo.preTransform = preTransform;
    swapchainCreateInfo.compositeAlpha = compositeAlphaFlagBits;
    swapchainCreateInfo.presentMode = swapChainPresentMode;
    swapchainCreateInfo.clipped = true;
    swapchainCreateInfo.oldSwapchain = oldSwapchain;

    DUCK_DEMO_VULKAN_ASSERT(vkCreateSwapchainKHR(m_vulkanDevice, &swapchainCreateInfo, nullptr, &m_vulkanSwapchain));

    if (oldSwapchain != VK_NULL_HANDLE)
    {
        for (VkImageView imageView : m_vulkanSwapchainImageViews)
        {
            vkDestroyImageView(m_vulkanDevice, imageView, nullptr);
        }

        uint32_t imageCount;
        DUCK_DEMO_VULKAN_ASSERT(vkGetSwapchainImagesKHR(m_vulkanDevice, oldSwapchain, &imageCount, nullptr));

        m_vulkanSwapchainImageViews.clear();

        vkDestroySwapchainKHR(m_vulkanDevice, oldSwapchain, nullptr);
    }

    m_vulkanSwapchainWidth = swapChainSize.width;
    m_vulkanSwapchainHeight = swapChainSize.height;
    m_vulkanSwapchainPixelFormat = selectedSurfaceFormat.format;

    DUCK_DEMO_VULKAN_ASSERT(vkGetSwapchainImagesKHR(m_vulkanDevice, m_vulkanSwapchain, &swapChainImageCount, nullptr));

    std::vector<VkImage> swapchainImages(swapChainImageCount);
    DUCK_DEMO_VULKAN_ASSERT(vkGetSwapchainImagesKHR(m_vulkanDevice, m_vulkanSwapchain, &swapChainImageCount, swapchainImages.data()));

    for (uint32_t i = 0; i < swapChainImageCount; ++i)
    {
        VkImageViewCreateInfo imageViewCreateInfo;
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.pNext = nullptr;
        imageViewCreateInfo.flags = 0;
        imageViewCreateInfo.image = swapchainImages[i];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = m_vulkanSwapchainPixelFormat;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1; 

        VkImageView imageView;
        DUCK_DEMO_VULKAN_ASSERT(vkCreateImageView(m_vulkanDevice, &imageViewCreateInfo, nullptr, &imageView));

        m_vulkanSwapchainImageViews.push_back(imageView);
    }

    return true;
}

bool Game::InitVulkanGameResources()
{
    VkSemaphoreCreateInfo semaphoreCreateInfo;
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = nullptr;
    semaphoreCreateInfo.flags = 0;
    DUCK_DEMO_VULKAN_ASSERT(vkCreateSemaphore(m_vulkanDevice, &semaphoreCreateInfo, s_allocator, &m_vulkanAquireSwapchain));
    DUCK_DEMO_VULKAN_ASSERT(vkCreateSemaphore(m_vulkanDevice, &semaphoreCreateInfo, s_allocator, &m_vulkanReleaseSwapchain));

    VkCommandPoolCreateInfo commandPoolCreateInfo;
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.pNext = nullptr;
    commandPoolCreateInfo.flags = 0;
    commandPoolCreateInfo.queueFamilyIndex = m_vulkanGraphicsQueueIndex;
    DUCK_DEMO_VULKAN_ASSERT(vkCreateCommandPool(m_vulkanDevice, &commandPoolCreateInfo, s_allocator, &m_vulkanPrimaryCommandPool));

    VkCommandBufferAllocateInfo commandBufferAllocateInfo;
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.pNext = nullptr;
    commandBufferAllocateInfo.commandPool = m_vulkanPrimaryCommandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;
    DUCK_DEMO_VULKAN_ASSERT(vkAllocateCommandBuffers(m_vulkanDevice, &commandBufferAllocateInfo, &m_vulkanPrimaryCommandBuffer));

    VkFenceCreateInfo fenceCreateInfo;
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.pNext = nullptr;
    fenceCreateInfo.flags = 0;
    DUCK_DEMO_VULKAN_ASSERT(vkCreateFence(m_vulkanDevice, &fenceCreateInfo, s_allocator, &m_vulkanSubmitFence));

    m_vulkanClearValue.color = {{ 0.392156869f, 0.58431375f, 0.929411769f, 1.0f }};

    return true;
}

bool Game::BeginRender()
{   
    VkResult acquireImageResult = vkAcquireNextImageKHR(m_vulkanDevice, m_vulkanSwapchain, UINT64_MAX, m_vulkanAquireSwapchain, VK_NULL_HANDLE, &m_currentSwapchainImageIndex);
    if (acquireImageResult != VK_SUCCESS && acquireImageResult != VK_SUBOPTIMAL_KHR && acquireImageResult != VK_ERROR_OUT_OF_DATE_KHR)
    {
        // if it's VK_SUBOPTIMAL_KHR or VK_ERROR_OUT_OF_DATE_KHR we'll rebuild the swap chain after the vkQueuePresentKHR
        // else every other error result besides VK_SUCCESS is considered an unhandled error
        DUCK_DEMO_VULKAN_ASSERT(acquireImageResult);
    }

    DUCK_DEMO_VULKAN_ASSERT(vkResetCommandPool(m_vulkanDevice, m_vulkanPrimaryCommandPool, 0));

    VkCommandBufferBeginInfo commandBufferBeginInfo;
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.pNext = nullptr;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    commandBufferBeginInfo.pInheritanceInfo = nullptr;
    DUCK_DEMO_VULKAN_ASSERT(vkBeginCommandBuffer(m_vulkanPrimaryCommandBuffer, &commandBufferBeginInfo));

    return true;
}

void Game::EndRender()
{
    DUCK_DEMO_VULKAN_ASSERT(vkEndCommandBuffer(m_vulkanPrimaryCommandBuffer));

    VkSubmitInfo submitInfo;
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &m_vulkanAquireSwapchain;
    const VkPipelineStageFlags waitPipelineStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submitInfo.pWaitDstStageMask = &waitPipelineStageFlags;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_vulkanPrimaryCommandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &m_vulkanReleaseSwapchain;
    DUCK_DEMO_VULKAN_ASSERT(vkQueueSubmit(m_vulkanQueue, 1, &submitInfo, m_vulkanSubmitFence));

    VkPresentInfoKHR presentInfo;
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &m_vulkanReleaseSwapchain;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_vulkanSwapchain;
    presentInfo.pImageIndices = &m_currentSwapchainImageIndex;
    presentInfo.pResults = nullptr;

    const VkResult queuePresentResult = vkQueuePresentKHR(m_vulkanQueue, &presentInfo);
    if (queuePresentResult == VK_SUBOPTIMAL_KHR || queuePresentResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        Resize();
    }
    else
    {
        DUCK_DEMO_VULKAN_ASSERT(queuePresentResult);
    }

    DUCK_DEMO_VULKAN_ASSERT(vkWaitForFences(m_vulkanDevice, 1, &m_vulkanSubmitFence, VK_TRUE, UINT64_MAX));
    vkResetFences(m_vulkanDevice, 1, &m_vulkanSubmitFence);
}

VkResult Game::CompileShaderFromDisk(const std::string& path, const shaderc_shader_kind shaderKind, VkShaderModule* OutShaderModule)
{
    std::unique_ptr<DuckDemoFile> shaderFile = DuckDemoUtils::LoadFileFromDisk(path);
    DUCK_DEMO_ASSERT(shaderFile);
    if (shaderFile == nullptr)
    {
        return VK_ERROR_UNKNOWN;
    }

    shaderc_compilation_result_t result = shaderc_compile_into_spv(
        m_shaderCompiler,
        shaderFile->buffer.get(),
        shaderFile->bufferSize, shaderKind,
        "Shader.file", "main", nullptr);

    DUCK_DEMO_ASSERT(result);
    if (result == nullptr)
    {
        return VK_ERROR_UNKNOWN;
    }

    const shaderc_compilation_status status = shaderc_result_get_compilation_status(result);
    if (status != shaderc_compilation_status_success)
    {
        const char* errorMessage = shaderc_result_get_error_message(result);
        if (errorMessage)
        {
            DUCK_DEMO_SHOW_ERROR("Shader Compile Error", errorMessage);
        }
        DUCK_DEMO_ASSERT(status == shaderc_compilation_status_success);

        shaderc_result_release(result);
        return VK_ERROR_UNKNOWN;
    }

    const uint32_t* shaderData = reinterpret_cast<const uint32_t*>(shaderc_result_get_bytes(result));
    size_t shaderDataSize = shaderc_result_get_length(result);

    constexpr uint32_t SPIRV_MAGIC = 0x07230203;
    if (SPIRV_MAGIC != shaderData[0])
    {
        shaderc_result_release(result);
        // shader data did not start with spir-v magic value
        return VK_ERROR_UNKNOWN;
    }

    VkShaderModuleCreateInfo shaderModuleCreateInfo;
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.pNext = nullptr;
    shaderModuleCreateInfo.flags = 0;
    shaderModuleCreateInfo.codeSize = shaderDataSize;
    shaderModuleCreateInfo.pCode = shaderData;

    VkResult vulkanResult = vkCreateShaderModule(m_vulkanDevice, &shaderModuleCreateInfo, s_allocator, OutShaderModule);
    DUCK_DEMO_VULKAN_ASSERT(vulkanResult);

    shaderc_result_release(result);

    return vulkanResult;
}

int32_t Game::FindMemoryByFlagAndType(const VkMemoryPropertyFlagBits memoryFlagBits, const uint32_t memoryTypeBits) const
{
    VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(m_vulkanPhysicalDevice, &physicalDeviceMemoryProperties);
    for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; ++i)
    {
        VkMemoryType memoryType = physicalDeviceMemoryProperties.memoryTypes[i];
        VkMemoryPropertyFlags memoryPropertyFlags = memoryType.propertyFlags;
        if ((memoryTypeBits & (1<<i)) != 0)
        {
            if ((memoryPropertyFlags & memoryFlagBits) != 0)
            {
                return i;
            }
        }
    }

    const std::string errorString = DuckDemoUtils::format("Could not find given memory flag (0x%08x) and type (0x%08x))", memoryFlagBits, memoryFlagBits);
    DUCK_DEMO_SHOW_ERROR("Critical Vulkan Error", errorString);
    Game::Get()->QuitGame();
    return -1;
}

VkResult Game::CreateVulkanBuffer(const VkDeviceSize deviceSize, const VkBufferUsageFlagBits bufferUsageFlagBits, VulkanBuffer& OutVulkanBuffer)
{
    OutVulkanBuffer.Reset();

    VkBufferCreateInfo bufferCreateInfo;
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.pNext = nullptr;
    bufferCreateInfo.flags = 0;
    bufferCreateInfo.size = deviceSize;
    bufferCreateInfo.usage = bufferUsageFlagBits;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferCreateInfo.queueFamilyIndexCount = 0;
    bufferCreateInfo.pQueueFamilyIndices = nullptr;

    VkResult result = vkCreateBuffer(m_vulkanDevice, &bufferCreateInfo, s_allocator, &OutVulkanBuffer.m_buffer);
    DUCK_DEMO_VULKAN_ASSERT(result);
    if (result != VK_SUCCESS)
    {
        return result;
    }

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(m_vulkanDevice, OutVulkanBuffer.m_buffer, &memoryRequirements);

    VkMemoryAllocateInfo memoryAllocateInfo;
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.pNext = nullptr;
    memoryAllocateInfo.allocationSize = memoryRequirements.size;

    const int32_t memoryTypeIndex = FindMemoryByFlagAndType(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, memoryRequirements.memoryTypeBits);
    if (memoryTypeIndex < 0)
    {
        return result;
    }
    memoryAllocateInfo.memoryTypeIndex = static_cast<uint32_t>(memoryTypeIndex);

    result = vkAllocateMemory(m_vulkanDevice, &memoryAllocateInfo, s_allocator, &OutVulkanBuffer.m_deviceMemory);
    DUCK_DEMO_VULKAN_ASSERT(result);
    if (result != VK_SUCCESS)
    {
        return result;
    }

    result = vkBindBufferMemory(m_vulkanDevice, OutVulkanBuffer.m_buffer, OutVulkanBuffer.m_deviceMemory, 0);
    DUCK_DEMO_VULKAN_ASSERT(result);
    if (result != VK_SUCCESS)
    {
        return result;
    }

    OutVulkanBuffer.m_deviceSize = deviceSize;

    return result;
}

void Game::FillVulkanBuffer(VulkanBuffer& vulkanBuffer, const void* data, const std::size_t dataSize)
{
    //vkCmdUpdateBuffer( CommandBuffer, myBuffer.buffer, 0, myBuffer.size, data );
    //command buffer version

    DUCK_DEMO_ASSERT(dataSize == vulkanBuffer.m_deviceSize); // this function expects the buffer and data size to match

    void* gpuMemory = nullptr;
    vkMapMemory(m_vulkanDevice, vulkanBuffer.m_deviceMemory, 0, VK_WHOLE_SIZE, 0, &gpuMemory);
    memcpy(gpuMemory, data, std::min(dataSize, vulkanBuffer.m_deviceSize));
    vkUnmapMemory(m_vulkanDevice, vulkanBuffer.m_deviceMemory);
}
