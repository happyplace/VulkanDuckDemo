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

    std::vector<const char*> extensionNames(extensionCount);
    if (!SDL_Vulkan_GetInstanceExtensions(m_window, &extensionCount, extensionNames.data()))
    {
        DUCK_DEMO_SHOW_ERROR("Vulkan Initialization Fail", "Failed to get instance extension names");
        return false;
    }

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

    extensionNames.push_back("VK_KHR_portability_enumeration");
    extensionNames.push_back("VK_KHR_get_physical_device_properties2");
#endif // DUCK_DEMO_VULKAN_PORTABILITY

#ifdef DUCK_DEMO_VULKAN_DEBUG
    extensionNames.push_back("VK_EXT_debug_report");
#endif // DUCK_DEMO_VULKAN_DEBUG

    instanceInfo.enabledExtensionCount = static_cast<uint32_t>(extensionNames.size());
    instanceInfo.ppEnabledExtensionNames = extensionNames.data();

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
