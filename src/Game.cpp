#include "Game.h"

#include <vector>

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

    if (m_instance)
    {
        if (m_vulkanSurface)
        {
            vkDestroySurfaceKHR(m_instance, m_vulkanSurface, s_allocator);
        }

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
    }

    return 0;
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

    VkPhysicalDevice gpu;
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
                    gpu = gpus[i];
                    graphicsQueueIndex = k;
                    deviceTypeScore = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
                    break;
                }
                else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU && deviceTypeScore < VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
                {
                    gpu = gpus[i];
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
    DUCK_DEMO_VULKAN_ASSERT(vkEnumerateDeviceExtensionProperties(gpu, nullptr, &deviceExtensionCount, nullptr));

    std::vector<VkExtensionProperties> deviceExtensions(deviceExtensionCount);
    DUCK_DEMO_VULKAN_ASSERT(vkEnumerateDeviceExtensionProperties(gpu, nullptr, &deviceExtensionCount, deviceExtensions.data()));

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

    DUCK_DEMO_VULKAN_ASSERT(vkCreateDevice(gpu, &deviceCreateInfo, s_allocator, &m_vulkanDevice));
    vkGetDeviceQueue(m_vulkanDevice, m_graphicsQueueIndex, 0, &m_vulkanQueue);

    return true;
}
