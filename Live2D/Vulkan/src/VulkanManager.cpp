#include "VulkanManager.hpp"
#include <cstdint>
#include <iostream>
#include <cstring>
#include <algorithm>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "Log.hpp"

#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#endif

#ifdef CSM_TARGET_LINUX_GL
#define VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME "VK_KHR_wayland_surface"
#define VK_KHR_XCB_SURFACE_EXTENSION_NAME "VK_KHR_xcb_surface"
#define VK_KHR_XLIB_SURFACE_EXTENSION_NAME "VK_KHR_xlib_surface"
#endif
/**
 * @brief   平台标准的 surface 创建（未设置外部钩子时使用）
 *          仅支持 Windows；其他平台编译期直接报错
 */
static VkSurfaceKHR createSurfaceInternal(VkInstance instance, const SurfaceCreateContext* ctx) {
    switch (ctx->platform) {

        case Platform::PLATFORM_WINDOWS: {
#if defined(_WIN32)
            if (!ctx->window) {
                LOGE("Win32 surface: window is null");
                return VK_NULL_HANDLE;
            }
            VkWin32SurfaceCreateInfoKHR createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
            createInfo.hinstance = GetModuleHandle(nullptr);
            createInfo.hwnd = static_cast<HWND>(ctx->window);

            VkSurfaceKHR surface = VK_NULL_HANDLE;
            if (vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS) {
                LOGE("failed to create Win32 surface!");
                return VK_NULL_HANDLE;
            }
            return surface;
#else
            LOGE("PLATFORM_WINDOWS requested but Win32 surface not compiled");
            return VK_NULL_HANDLE;
#endif
        }

        case Platform::PLATFORM_XCB: {
#if defined(__linux__)
            if (!ctx->connection || !ctx->window) {
                LOGE("XCB surface: connection or window is null");
                return VK_NULL_HANDLE;
            }
            VkXcbSurfaceCreateInfoKHR createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
            createInfo.connection = static_cast<xcb_connection_t*>(ctx->connection);
            createInfo.window = static_cast<xcb_window_t>(reinterpret_cast<uintptr_t>(ctx->window));

            VkSurfaceKHR surface = VK_NULL_HANDLE;
            if (vkCreateXcbSurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS) {
                LOGE("failed to create XCB surface!");
                return VK_NULL_HANDLE;
            }
            return surface;
#else
            LOGE("PLATFORM_XCB requested but VK_USE_PLATFORM_XCB_KHR not compiled");
            return VK_NULL_HANDLE;
#endif
        }

        case Platform::PLATFORM_XLIB: {
#if defined(__linux__)
            LOGI("Platform: xlib");
            if (!ctx->display || !ctx->window) {
                LOGE("Xlib surface: display or window is null");
                return VK_NULL_HANDLE;
            }
            VkXlibSurfaceCreateInfoKHR createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
            createInfo.dpy = static_cast<Display*>(ctx->display);
            createInfo.window = static_cast<Window>(reinterpret_cast<uintptr_t>(ctx->window));

            VkSurfaceKHR surface = VK_NULL_HANDLE;
            if (vkCreateXlibSurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS) {
                LOGE("failed to create Xlib surface!");
                return VK_NULL_HANDLE;
            }
            return surface;
#else
            LOGE("PLATFORM_XLIB requested but VK_USE_PLATFORM_XLIB_KHR not compiled");
            return VK_NULL_HANDLE;
#endif
        }

        case Platform::PLATFORM_WAYLAND: {
#if defined(__linux__)
            if (!ctx->display || !ctx->window) {
                LOGE("Wayland surface: display or surface is null");
                return VK_NULL_HANDLE;
            }
            VkWaylandSurfaceCreateInfoKHR createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
            createInfo.display = static_cast<wl_display*>(ctx->display);
            createInfo.surface = static_cast<wl_surface*>(ctx->window);

            VkSurfaceKHR surface = VK_NULL_HANDLE;
            if (vkCreateWaylandSurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS) {
                LOGE("failed to create Wayland surface!");
                return VK_NULL_HANDLE;
            }
            return surface;
#else
            LOGE("PLATFORM_WAYLAND requested but VK_USE_PLATFORM_WAYLAND_KHR not compiled");
            return VK_NULL_HANDLE;
#endif
        }

        default:
            LOGF("unknown platform backend: %u", static_cast<unsigned>(ctx->platform));
            return VK_NULL_HANDLE;
    }
}

/**
* @brief   デバッグ用コールバック
*/
static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void*) {
    std::cout << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

/**
* @brief   デバッグ用拡張関数を使えるようにする
*/
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
                                      const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator,
                                      VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

/**
* @brief   デバッグメッセンジャーの破棄
*/
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

static VulkanManager* sInstance = nullptr;

VulkanManager* VulkanManager::GetInstance() {
    if (sInstance == nullptr) {
        sInstance = new VulkanManager(nullptr);
    }
    return sInstance;
}

VulkanManager::VulkanManager(void* wind) :
    _instance(VK_NULL_HANDLE), _physicalDevice(VK_NULL_HANDLE), _device(VK_NULL_HANDLE),
    _swapchainManager(), _surface(), _graphicQueue(VK_NULL_HANDLE), _presentQueue(VK_NULL_HANDLE),
    _commandPool(VK_NULL_HANDLE), _debugMessenger(), _frameWidth(1), _frameHeight(1) {
    _surfaceCtx.window = wind;
}

VulkanManager::~VulkanManager() {
    Destroy();
}

bool VulkanManager::CheckValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (uint32_t i = 0; i < availableLayers.size(); i++) {
            if (strcmp(layerName, availableLayers[i].layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

void VulkanManager::SetInstanceExtensions(const char** extensions, uint32_t count) {
    _instanceExtensions.clear();
    for (uint32_t i = 0; i < count; i++) {
        _instanceExtensions.push_back(extensions[i]);
    }
}

std::vector<const char*> VulkanManager::GetRequiredExtensions() {
    std::vector<const char*> extensions;
    for (uint32_t i = 0; i < _instanceExtensions.size(); i++) {
        extensions.push_back(_instanceExtensions[i]);
    }

    // 自动追加平台 surface 扩展（排除外部已注入的重复项）
    auto hasExt = [&](const char* name) {
        return std::find(extensions.begin(), extensions.end(), std::string(name)) !=
               extensions.end();
    };

    if (!hasExt(VK_KHR_SURFACE_EXTENSION_NAME)) {
        extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    }

#if defined(_WIN32)
    if (!hasExt(VK_KHR_WIN32_SURFACE_EXTENSION_NAME)) {
        extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
    }
#elif defined(__linux__)
    bool anySurface = false;
    auto addExtensionIfNeeded = [&](const char* target) {
        if (!hasExt(target)) {
            extensions.push_back(target);
            anySurface = true;
            LOGD("surface ext: %s", target);
        }
    };

    switch (_surfaceCtx.platform) {
        case PLATFORM_XLIB:
            addExtensionIfNeeded(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
            break;
        case PLATFORM_WAYLAND:
            addExtensionIfNeeded(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
            break;
        case PLATFORM_XCB:
            addExtensionIfNeeded(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
            break;
    }
    // 编译了哪个后端，就尝试启用哪个；实例创建时还不知道上层会传什么 backend，
    // 所以 Wayland 和 X11 只要编译进来了就都尝试启用，为运行期选择铺路。

    if (!anySurface) {
        LOGE("no platform surface extension available (Wayland/X11)");
        return {};
    }

#else
#error "GetRequiredExtensions: unsupported platform"
#endif

    if (_enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

void VulkanManager::populateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

void VulkanManager::CreateInstance() {
    //検証レイヤーが有効のときに使えるか確認
    if (_enableValidationLayers && !CheckValidationLayerSupport()) {
        //検証レイヤーをサポートしていないのでログを出して使えないようにする
        LOGE("validation layers requested, but not available!");
        _enableValidationLayers = false;
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "SAMPLE";
    appInfo.pEngineName = "SAMPLE";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    std::vector<const char*> extensions = GetRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (_enableValidationLayers) {
        createInfo.enabledLayerCount =
            static_cast<uint32_t>(sizeof(validationLayers) / sizeof(validationLayers[0]));
        createInfo.ppEnabledLayerNames = validationLayers;
        // vkDestroyInstanceでデバッグコールが呼ばれるために次の行が必要
        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
    }
    if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS) {
        LOGE("failed to create _instance!");
    }
}

void VulkanManager::SetupDebugMessenger() {
    if (!_enableValidationLayers)
        return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(_instance, &createInfo, nullptr, &_debugMessenger) !=
        VK_SUCCESS) {
        LOGE("failed to set up debug messenger!");
    }
}

bool VulkanManager::CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(
        physicalDevice, nullptr, &extensionCount, availableExtensions.data());

    for (uint32_t i = 0; i < sizeof(deviceExtensions) / sizeof(deviceExtensions[0]); i++) {
        bool found = false;
        for (uint32_t j = 0; j < availableExtensions.size(); j++) {
            if (strcmp(deviceExtensions[i], availableExtensions[j].extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (found == false) {
            return false;
        }
    }
    return true;
}

void VulkanManager::CreateSurface() {
    if (_createSurfaceFunc != nullptr) {
        _surface = _createSurfaceFunc(_instance, &_surfaceCtx);
    } else {
        _surface = createSurfaceInternal(_instance, &_surfaceCtx);
    }

    if (_surface == VK_NULL_HANDLE) {
        LOGE("failed to create window surface!");
    }
}

void VulkanManager::FindQueueFamilies(VkPhysicalDevice device) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    for (uint32_t i = 0; i < queueFamilies.size(); i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _surface, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }
    }
}

bool VulkanManager::IsDeviceSuitable(VkPhysicalDevice device) {
    FindQueueFamilies(device);
    bool extensionsSupported = CheckDeviceExtensionSupport(device);
    bool swapChainAdequate = false;
    //デバイスの拡張機能がサポートされていたら、スワップチェインをサポートしているか確認する
    if (extensionsSupported) {
        SwapchainManager::SwapchainSupportDetails swapchainSupport =
            SwapchainManager::QuerySwapchainSupport(device, _surface);
        swapChainAdequate =
            swapchainSupport.formats.size() != 0 && swapchainSupport.presentModes.size() != 0;
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return indices.isComplete() && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

void VulkanManager::PickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        LOGE("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

    VkPhysicalDeviceProperties properties{};
    VkPhysicalDevice bestMatch = VK_NULL_HANDLE;
    // 遍历所有物理设备，选择符合条件的设备
    for (uint32_t i = 0; i < devices.size(); i++) {
        if (IsDeviceSuitable(devices[i])) {
            // 优先使用独立显卡
            vkGetPhysicalDeviceProperties(devices[i], &properties);
            if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                bestMatch = devices[i];
                break;
            }
            // 默认选择第一个符合条件的设备，走原逻辑
            if (bestMatch == VK_NULL_HANDLE) {
                bestMatch = devices[i];
            }
        }
    }
    _physicalDevice = bestMatch;
    LOGI("Device: %s", properties.deviceName);
    if (_physicalDevice == VK_NULL_HANDLE) {
        LOGE("failed to find a suitable GPU!");
    }
}

void VulkanManager::CreateLogicalDevice() {
    FindQueueFamilies(_physicalDevice);

    std::vector<int32_t> uniqueQueueFamilies;
    if (indices.graphicsFamily == indices.presentFamily) {
        uniqueQueueFamilies.push_back(indices.graphicsFamily);
    } else {
        uniqueQueueFamilies.push_back(indices.graphicsFamily);
        uniqueQueueFamilies.push_back(indices.presentFamily);
    }

    float queuePriority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    for (uint32_t i = 0; i < uniqueQueueFamilies.size(); i++) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = uniqueQueueFamilies[i];
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.enabledExtensionCount = sizeof(deviceExtensions) / sizeof(deviceExtensions[0]);
    createInfo.ppEnabledExtensionNames = deviceExtensions;

    // Device Layers have never worked since Vulkan 1.0 and
    // only Instance Layers should be used instead: 
    // https://docs.vulkan.org/spec/latest/appendices/legacy.html#legacy-devicelayers.
    createInfo.enabledLayerCount = 0;

    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT dynamicStateF{};
    dynamicStateF.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
    dynamicStateF.extendedDynamicState = VK_TRUE;
    dynamicStateF.pNext = nullptr;

    VkPhysicalDeviceVulkan13Features deviceVulkan13Features{};
    deviceVulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    deviceVulkan13Features.synchronization2 = VK_TRUE;
    deviceVulkan13Features.dynamicRendering = VK_TRUE;
    deviceVulkan13Features.pNext = &dynamicStateF;

    VkPhysicalDeviceFeatures2 deviceFeatures2{};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.pNext = &deviceVulkan13Features;
    vkGetPhysicalDeviceFeatures2(_physicalDevice, &deviceFeatures2);
    createInfo.pNext = &deviceFeatures2;

    if (vkCreateDevice(_physicalDevice, &createInfo, nullptr, &_device) != VK_SUCCESS) {
        LOGE("failed to create logical _device!");
    }

    // キューハンドルを取得
    vkGetDeviceQueue(_device, indices.graphicsFamily, 0, &_graphicQueue);
    vkGetDeviceQueue(_device, indices.presentFamily, 0, &_presentQueue);
}

void VulkanManager::ChooseSupportedDepthFormat() {
    VkFormat depthFormats[5] = {VK_FORMAT_D32_SFLOAT_S8_UINT,
                                VK_FORMAT_D32_SFLOAT,
                                VK_FORMAT_D24_UNORM_S8_UINT,
                                VK_FORMAT_D16_UNORM_S8_UINT,
                                VK_FORMAT_D16_UNORM};

    for (int32_t i = 0; i < sizeof(depthFormats) / sizeof(depthFormats[0]); i++) {
        VkFormatProperties formatProps;
        vkGetPhysicalDeviceFormatProperties(_physicalDevice, depthFormats[i], &formatProps);

        if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            _depthFormat = depthFormats[i];
            return;
        }
    }
    LOGE("can't find depth format!");
    _depthFormat = depthFormats[0];
}

void VulkanManager::CreateCommandPool() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = indices.graphicsFamily;

    if (vkCreateCommandPool(_device, &poolInfo, nullptr, &_commandPool) != VK_SUCCESS) {
        LOGE("failed to create graphics command pool!");
    }
}

void VulkanManager::RecreateFinishSemaphores() {
    for (auto sem : _imageFinishDrawSemaphores) {
        if (sem != VK_NULL_HANDLE) {
            vkDestroySemaphore(_device, sem, nullptr);
        }
    }

    const int newCount = _swapchainManager->GetImageCount();
    if (!_imageFinishDrawSemaphores.empty() && (int)_imageFinishDrawSemaphores.size() != newCount) {
        LOGW("swapchain image count changed (%d -> %d); renderer per-image resource "
             "counts are not rebuilt this frame",
             (int)_imageFinishDrawSemaphores.size(),
             newCount);
    }

    _imageFinishDrawSemaphores.assign(newCount, VK_NULL_HANDLE);
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for (auto& sem : _imageFinishDrawSemaphores) {
        vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &sem);
    }
}

void VulkanManager::CreateSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    RecreateFinishSemaphores();

    _imageAvailableSemaphores.resize(MAX_FRAME_IN_FLIGHT);
    _frameFences.resize(MAX_FRAME_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAME_IN_FLIGHT; i++) {
        vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_imageAvailableSemaphores[i]);
        vkCreateFence(_device, &fenceInfo, nullptr, &_frameFences[i]);
    }

    // per-slot 常驻命令缓冲（槽位由 fence 串行化，重录前必已等待该槽完成）
    _frameCmds.assign(MAX_FRAME_IN_FLIGHT, VK_NULL_HANDLE);
    _uploadCmds.assign(MAX_FRAME_IN_FLIGHT, VK_NULL_HANDLE);
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = _commandPool;
    allocInfo.commandBufferCount = 1;
    for (int i = 0; i < MAX_FRAME_IN_FLIGHT; i++) {
        if (vkAllocateCommandBuffers(_device, &allocInfo, &_frameCmds[i]) != VK_SUCCESS) {
            LOGE("failed to allocate frame command buffer!");
        }
        if (vkAllocateCommandBuffers(_device, &allocInfo, &_uploadCmds[i]) != VK_SUCCESS) {
            LOGE("failed to allocate upload command buffer!");
        }
    }
}

void VulkanManager::Initialize() {
    CreateInstance();
    SetupDebugMessenger();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    ChooseSupportedDepthFormat();
    ChooseSurfaceFormat();
    _swapchainManager = new SwapchainManager(
        _physicalDevice, _device, _surface, _surfaceFormat, indices.graphicsFamily, indices.presentFamily, {_frameWidth, _frameHeight});
    CreateCommandPool();
    // _swapchainManager->ChangeLayout(_device, _commandPool, _graphicQueue);
    CreateSyncObjects();
}

VkCommandBuffer VulkanManager::BeginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = _commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(_device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void VulkanManager::SubmitCommand(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);
    {
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_VERTEX_INPUT_BIT};
        submitInfo.pWaitDstStageMask = waitStages;
        vkQueueSubmit(_graphicQueue, 1, &submitInfo, VK_NULL_HANDLE);
    }

    // コマンドの実行終了まで待機
    vkQueueWaitIdle(_graphicQueue);
    vkFreeCommandBuffers(_device, _commandPool, 1, &commandBuffer);
}

void VulkanManager::UpdateDrawFrame() {
    // 只等待不重置：fence 的重置推迟到 EndFrame 提交前。
    // 若 acquire 失败（OUT_OF_DATE 等）本帧不会提交，fence 保持 signaled，
    // 下一帧同槽等待立即通过，避免“已 reset 但永不 submit”的死锁。
    vkWaitForFences(_device, 1, &_frameFences[_frameIndex], VK_TRUE, UINT64_MAX /* forever */);
    VkResult result = vkAcquireNextImageKHR(_device,
                                            _swapchainManager->GetSwapchain(),
                                            UINT64_MAX,
                                            _imageAvailableSemaphores[_frameIndex],
                                            VK_NULL_HANDLE,
                                            &_imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        _isSwapchainInvalid = true;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        LOGE("failed to acquire swap chain image!");
    }
}

void VulkanManager::PostDraw() {
    VkResult result = _swapchainManager->QueuePresent(
        _presentQueue, _imageIndex, _imageFinishDrawSemaphores[_imageIndex]);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _framebufferResized) {
        _framebufferResized = false;
        _isSwapchainInvalid = true;
    } else if (result != VK_SUCCESS) {
        LOGE("failed to present swap chain image!");
    }
    _frameIndex = (_frameIndex + 1) % MAX_FRAME_IN_FLIGHT;
}

void VulkanManager::RecreateSwapchain() {
    vkDeviceWaitIdle(_device);
    _swapchainManager->Cleanup(_device);
    _swapchainManager->CreateSwapchain(_physicalDevice, _device, _surface, indices.graphicsFamily, indices.presentFamily, 
            {_frameWidth, _frameHeight});
    // _swapchainManager->ChangeLayout(_device, _commandPool, _graphicQueue);
    // 图像数量可能变化，per-image 呈现完成信号量必须随之重建
    RecreateFinishSemaphores();
}

// ---- 帧解耦接口实现 ----

bool VulkanManager::BeginFrame() {
#if defined(TRACY_ENABLE)
    ZoneScopedN("vk/BeginFrame");
#endif
    if (!IsInitialized()) {
        LOGE("VulkanManager is not initialized");
        return false;
    }

    if (_frameActive) {
        LOGE("BeginFrame called while a frame is already active");
        return false;
    }

    // 上一帧 present 后标记的失效：先重建，让调用方本帧跳过
    if (_isSwapchainInvalid) {
        RecreateSwapchain();
        _isSwapchainInvalid = false;
        return false;
    }

    // 获取 swapchain 图像；失效时先重建，让调用方本帧跳过
    {
#if defined(TRACY_ENABLE)
        ZoneScopedN("vk/acquire");
#endif
        UpdateDrawFrame();
    }
    if (_isSwapchainInvalid) {
        _isSwapchainInvalid = false;
        RecreateSwapchain();
        return false;
    }

    // per-slot 常驻命令缓冲（CreateSyncObjects 已分配）。
    // 本槽 fence 已在 UpdateDrawFrame 等待过（该槽上一帧 GPU 工作已完成），
    // 此时 reset/re-record 本槽缓冲是安全的
    VkCommandBuffer frameCmd = _frameCmds[_frameIndex];
    VkCommandBuffer uploadCmd = _uploadCmds[_frameIndex];
    vkResetCommandBuffer(frameCmd, 0);
    vkResetCommandBuffer(uploadCmd, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(frameCmd, &beginInfo);
    vkBeginCommandBuffer(uploadCmd, &beginInfo);

    // 布局迁移：UNDEFINED → COLOR_ATTACHMENT_OPTIMAL（present 后布局视为 UNDEFINED）
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = _swapchainManager->GetImages()[_imageIndex];
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    vkCmdPipelineBarrier(frameCmd,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         0,
                         0,
                         nullptr,
                         0,
                         nullptr,
                         1,
                         &barrier);

    // 开始主 rendering 区间：清屏为透明（premultiplied-alpha 下正确叠加模型）
    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = _swapchainManager->GetImageViews()[_imageIndex];
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = {{0.0f, 1.0f, 0.0f, 0.0f}};

    VkExtent2D extent = _swapchainManager->GetExtent();
    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset = {0, 0};
    renderingInfo.renderArea.extent = extent;
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;
    vkCmdBeginRendering(frameCmd, &renderingInfo);
    LOGD("swapchain extent=%ux%u", extent.width, extent.height);
    _frameActive = true;
    return true;
}

void VulkanManager::EndFrame() {
#if defined(TRACY_ENABLE)
    ZoneScopedN("vk/EndFrame");
#endif
    if (!_frameActive) {
        LOGE("EndFrame called without an active frame");
        return;
    }
    _frameActive = false;

    VkCommandBuffer frameCmd = _frameCmds[_frameIndex];
    VkCommandBuffer uploadCmd = _uploadCmds[_frameIndex];

    // 结束主 rendering 区间
    vkCmdEndRendering(frameCmd);

    // 布局迁移：COLOR_ATTACHMENT_OPTIMAL → PRESENT_SRC_KHR
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = _swapchainManager->GetImages()[_imageIndex];
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = 0;
    vkCmdPipelineBarrier(frameCmd,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         0,
                         0,
                         nullptr,
                         0,
                         nullptr,
                         1,
                         &barrier);

    vkEndCommandBuffer(frameCmd);

    // 传输缓冲结束前补一条可见性屏障：帧缓冲里的顶点/索引读取要看到这里的拷贝结果
    {
        VkMemoryBarrier memoryBarrier{};
        memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        memoryBarrier.dstAccessMask =
            VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT;
        vkCmdPipelineBarrier(uploadCmd,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_VERTEX_INPUT_BIT|VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                             0,
                             1,
                             &memoryBarrier,
                             0,
                             nullptr,
                             0,
                             nullptr);
    }
    vkEndCommandBuffer(uploadCmd);

    // 提交：等待帧首 acquire 发出的 imageAvailable 信号量
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    // 传输缓冲先于帧缓冲执行（同队列按顺序）
    VkCommandBuffer submitBuffers[] = {uploadCmd, frameCmd};
    submitInfo.commandBufferCount = 2;
    submitInfo.pCommandBuffers = submitBuffers;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &_imageAvailableSemaphores[_frameIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &_imageFinishDrawSemaphores[_imageIndex];
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.pWaitDstStageMask = waitStages;
    {
#if defined(TRACY_ENABLE)
        ZoneScopedN("vk/submit");
#endif
        // 确认本槽 fence 已由上一帧置位后（UpdateDrawFrame 已等待），
        // 在提交前重置，本帧 submit 完成时再次置位
        vkResetFences(_device, 1, &_frameFences[_frameIndex]);
        if (VK_SUCCESS != vkQueueSubmit(_graphicQueue, 1, &submitInfo, _frameFences[_frameIndex])) {
            LOGW("frame %u failed to submit image", _frameIndex);
        }
    }

    {
#if defined(TRACY_ENABLE)
        ZoneScopedN("vk/present");
#endif
        PostDraw();
    }

#if defined(TRACY_ENABLE)
    FrameMark;
#endif
}

void VulkanManager::SuspendMainRegion() {
    if (!_frameActive) {
        LOGE("SuspendMainRegion called without an active frame");
        return;
    }
    vkCmdEndRendering(_frameCmds[_frameIndex]);
}

void VulkanManager::ResumeMainRegion() {
    if (!_frameActive) {
        LOGE("ResumeMainRegion called without an active frame");
        return;
    }

    // 挂起期间图像布局保持 COLOR_ATTACHMENT_OPTIMAL，恢复区间无需 barrier；
    // loadOp=LOAD 保留挂起前已绘制的内容
    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = _swapchainManager->GetImageViews()[_imageIndex];
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkExtent2D extent = _swapchainManager->GetExtent();
    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset = {0, 0};
    renderingInfo.renderArea.extent = extent;
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;
    vkCmdBeginRendering(_frameCmds[_frameIndex], &renderingInfo);
}

uint32_t VulkanManager::GetFramesInFlight() const {
    if (_swapchainManager == nullptr) {
        return 0;
    }
    return static_cast<uint32_t>(MAX_FRAME_IN_FLIGHT);
}

void VulkanManager::ResetCommandBuffers() {
    // 帧录制中重置命令池属于非法操作（会破坏正在录制的缓冲）
    if (!IsInitialized() || _frameActive) {
        return;
    }
    // 先等 GPU 完成所有已提交工作，再重置：重置会把池内所有命令缓冲恢复到
    // initial 状态，从而解除它们对 buffer/image 的引用
    vkDeviceWaitIdle(_device);
    vkResetCommandPool(_device, _commandPool, 0);
}

void VulkanManager::Destroy() {
    if (!IsInitialized()) {
        return;
    }
    vkDeviceWaitIdle(_device);
    vkQueueWaitIdle(_graphicQueue);

    // 释放帧解耦模式的常驻命令缓冲（随 command pool 一并销毁，仅复位指针）
    _frameCmds.clear();
    _uploadCmds.clear();
    _frameActive = false;

    if (_swapchainManager != NULL) {
        _swapchainManager->Cleanup(_device);
        delete _swapchainManager;
        _swapchainManager = NULL;
    }

    const int size = (int)_imageFinishDrawSemaphores.size();
    for (int i = 0; i < size; i++) {
        vkDestroySemaphore(_device, _imageFinishDrawSemaphores[i], nullptr);
    }
    for (int i = 0; i < MAX_FRAME_IN_FLIGHT; i++) {
        vkDestroySemaphore(_device, _imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(_device, _frameFences[i], nullptr);
    }
    _imageAvailableSemaphores.clear();
    _imageFinishDrawSemaphores.clear();
    _frameFences.clear();

    if (_enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(_instance, _debugMessenger, nullptr);
    }

    vkDestroyCommandPool(_device, _commandPool, nullptr);
    vkDestroyDevice(_device, nullptr);
    _device = VK_NULL_HANDLE;
    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkDestroyInstance(_instance, nullptr);
    _instance = VK_NULL_HANDLE;
}

void VulkanManager::ChooseSurfaceFormat() {
    uint32_t count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, _surface, &count, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, _surface, &count, formats.data());

    for (auto& format : formats) {
        if (format.format == _surfaceFormat || format.format == VK_FORMAT_B8G8R8A8_UNORM) {
            _surfaceFormat = format.format;
            LOGD("selected format: %d", _surfaceFormat);
            return;
        }
    }

    if (!formats.empty()) {
        _surfaceFormat = formats[0].format;
        LOGD("selected format: %d", _surfaceFormat);
    } else {
        LOGE("No supported surface format");
    }
}

void VulkanManager::InvalidateSizeIfNeeded(int width, int height) {
    auto extent = _swapchainManager->GetExtent();
    if (extent.width != width || extent.height != height) {
        _isSwapchainInvalid = true;
    }
    _frameWidth = width;
    _frameHeight = height;
}