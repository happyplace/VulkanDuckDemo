#include "Game.h"

#include <SDL_vulkan.h>

#include "DuckDemoUtils.h"

static VkAllocationCallbacks* s_allocator = nullptr;

Game* Game::ms_instance = nullptr;
Game* Game::Get()
{
    DUCK_DEMO_ASSERT(ms_instance);
    return ms_instance;
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

    if (m_vulkanAquireSwapchainImage)
    {
        vkDestroySemaphore(m_vulkanDevice, m_vulkanAquireSwapchainImage, s_allocator);
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

    if (m_instance)
    {
        vkDestroyInstance(m_instance, s_allocator);
    }

    if (m_vulkanDevice)
    {
        vkDestroyDevice(m_vulkanDevice, s_allocator);
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

    if (!InitVulkanInstance())
    {
        return 1;
    }

    if (!InitVulkanDevice())
    {
        return 1;
    }

    if (!InitVulkanSwapChain())
    {
        return 1;
    }

    OnResize();

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
        }

        m_gameTimer.Tick();

        Update();
    }

    return 0;
}

void Game::Update()
{
    uint32_t currentSwapchainImageIndex;
    VkResult acquireImageResult = vkAcquireNextImageKHR(m_vulkanDevice, m_vulkanSwapchain, UINT64_MAX, m_vulkanAquireSwapchainImage, VK_NULL_HANDLE, &currentSwapchainImageIndex);
    if (acquireImageResult == VK_SUBOPTIMAL_KHR || acquireImageResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        Resize();
        acquireImageResult = vkAcquireNextImageKHR(m_vulkanDevice, m_vulkanSwapchain, UINT64_MAX, m_vulkanAquireSwapchainImage, VK_NULL_HANDLE, &currentSwapchainImageIndex);
    }
    else
    {
        DUCK_DEMO_VULKAN_ASSERT(acquireImageResult);
    }

    if (acquireImageResult != VK_SUCCESS)
    {
        DUCK_DEMO_VULKAN_ASSERT(acquireImageResult);
        vkQueueWaitIdle(m_vulkanQueue);
        return;
    }

    OnUpdate(m_gameTimer);
}

void Game::Resize()
{
    DUCK_DEMO_ASSERT(m_vulkanPhysicalDevice != VK_NULL_HANDLE);

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    DUCK_DEMO_ASSERT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_vulkanPhysicalDevice, m_vulkanSurface, &surfaceCapabilities));

    if (surfaceCapabilities.currentExtent.width == m_vulkanSwapchainWidth &&
        surfaceCapabilities.currentExtent.height == m_vulkanSwapchainHeight)
    {
        return;
    }

    vkDeviceWaitIdle(m_vulkanDevice);

    InitVulkanSwapChain();

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

    m_window = SDL_CreateWindow("Vulkan Duck Demo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN);
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

    m_graphicsQueueIndex = static_cast<uint32_t>(graphicsQueueIndex);

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
    deviceCreateInfo.pEnabledFeatures = nullptr;

    DUCK_DEMO_VULKAN_ASSERT(vkCreateDevice(m_vulkanPhysicalDevice, &deviceCreateInfo, s_allocator, &m_vulkanDevice));
    vkGetDeviceQueue(m_vulkanDevice, m_graphicsQueueIndex, 0, &m_vulkanQueue);

    return true;
}

bool Game::InitVulkanSwapChain()
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
        int32_t windowWidth;
        int32_t windowHeight;
        SDL_GetWindowSize(m_window, &windowWidth, &windowHeight);

        swapChainSize.width = windowWidth;
        swapChainSize.height = windowHeight;
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

    VkSemaphoreCreateInfo semaphoreCreateInfo;
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = nullptr;
    semaphoreCreateInfo.flags = 0;
    DUCK_DEMO_VULKAN_ASSERT(vkCreateSemaphore(m_vulkanDevice, &semaphoreCreateInfo, s_allocator, &m_vulkanAquireSwapchainImage));

    return true;
}
