#include "SwapchainManager.hpp"
#include "Log.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <vulkan/vulkan_core.h>

SwapchainManager::SwapchainManager(void* _window, VkPhysicalDevice physicalDevice, VkDevice device,
                                   VkSurfaceKHR surface, VkFormat format, uint32_t graphicsFamily, uint32_t presentFamily)
                                   : swapchainFormat(format)
{
    CreateSwapchain(_window, physicalDevice, device, surface, graphicsFamily, presentFamily);
}

SwapchainManager::SwapchainSupportDetails SwapchainManager::QuerySwapchainSupport(
    VkPhysicalDevice& physicalDevice, VkSurfaceKHR surface)
{
    SwapchainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount,
                                                  details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR SwapchainManager::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (uint32_t i = 0; i < availableFormats.size(); i++)
    {
        if (availableFormats[i].format == swapchainFormat && availableFormats[i].colorSpace ==
            VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormats[i];
        }
    }
    //他に使えるフォーマットが無かったら最初のフォーマットを使う
    return availableFormats[0];
}

VkPresentModeKHR SwapchainManager::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    // 默认 FIFO（无撕裂、跟随刷新率），发布构建恒定此模式，可以有效降低cpu占用。
    // 仅 Tracy 构建（压测/录制）允许用环境变量关闭呈现节流：
    //   LIVE2D_VK_PRESENT_MODE=immediate | mailbox | fifo | fifo_relaxed
    VkPresentModeKHR preferred = VK_PRESENT_MODE_FIFO_KHR;
#if defined(TRACY_ENABLE)
    if (const char* requested = std::getenv("LIVE2D_VK_PRESENT_MODE"))
    {
        if (std::strcmp(requested, "immediate") == 0) preferred = VK_PRESENT_MODE_IMMEDIATE_KHR;
        else if (std::strcmp(requested, "fifo") == 0) preferred = VK_PRESENT_MODE_FIFO_KHR;
        else if (std::strcmp(requested, "fifo_relaxed") == 0) preferred = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
        else if (std::strcmp(requested, "mailbox") == 0) preferred = VK_PRESENT_MODE_MAILBOX_KHR;
        else LOGW("unknown LIVE2D_VK_PRESENT_MODE: %s, fallback to MAILBOX", requested);
    }
#endif

    const auto supports = [&availablePresentModes](VkPresentModeKHR mode) {
        return std::find(availablePresentModes.begin(), availablePresentModes.end(), mode) !=
               availablePresentModes.end();
    };

    if (supports(preferred))
    {
        return preferred;
    }
    if (supports(VK_PRESENT_MODE_MAILBOX_KHR))
    {
        return VK_PRESENT_MODE_MAILBOX_KHR;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapchainManager::ChooseSwapExtent(void*, const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        return capabilities.currentExtent;
    }
    else
    {
        //for Retina display
        int width = 0, height = 0;
        // glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        if (actualExtent.width < capabilities.minImageExtent.width)
        {
            actualExtent.width = capabilities.minImageExtent.width;
        }
        else if (actualExtent.width > capabilities.maxImageExtent.width)
        {
            actualExtent.width = capabilities.maxImageExtent.width;
        }

        if (actualExtent.height < capabilities.minImageExtent.height)
        {
            actualExtent.height = capabilities.minImageExtent.height;
        }
        else if (actualExtent.height > capabilities.maxImageExtent.height)
        {
            actualExtent.height = capabilities.maxImageExtent.height;
        }

        return actualExtent;
    }
}

void SwapchainManager::CreateSwapchain(void* window, VkPhysicalDevice physicalDevice, VkDevice device,
                                       VkSurfaceKHR surface, uint32_t graphicsFamily,
                                       uint32_t presentFamily)
{
    SwapchainSupportDetails swapChainSupport = QuerySwapchainSupport(physicalDevice, surface);
    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
    extent = ChooseSwapExtent(window, swapChainSupport.capabilities);

    imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (graphicsFamily != presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        uint32_t queueFamilyIndices[2] = {graphicsFamily, presentFamily};
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain) != VK_SUCCESS)
    {
        LOGE("failed to create swap chain");
    }

    LOGD("swapchain: presentMode=%d imageCount=%u (min=%u max=%u) format=%d colorSpace=%d "
         "extent=%ux%u sharing=%d",
         (int)presentMode, imageCount, swapChainSupport.capabilities.minImageCount,
         swapChainSupport.capabilities.maxImageCount, (int)surfaceFormat.format,
         (int)surfaceFormat.colorSpace, extent.width, extent.height,
         (int)createInfo.imageSharingMode);

    // swapchain imageを取得する
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    images.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, images.data());

    imageViews.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; i++)
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = images[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = swapchainFormat;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &viewInfo, nullptr, &imageViews[i]) != VK_SUCCESS)
        {
            LOGE("failed to create texture image view");
        }
    }
}

VkResult SwapchainManager::QueuePresent(VkQueue& queue, uint32_t imageIndex, VkSemaphore finishSemaphore)
{
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    VkSwapchainKHR swapChains[] = {swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &finishSemaphore;
    return vkQueuePresentKHR(queue, &presentInfo);
}

void SwapchainManager::ChangeLayout(VkDevice device, VkCommandPool commandPool, VkQueue queue)
{
    for (uint32_t i = 0; i < imageCount; i++)
    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.image = images[i];
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;

        VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage,
            destinationStage,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier
        );

        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }
}

void SwapchainManager::Cleanup(VkDevice device)
{
    if (swapchain != VK_NULL_HANDLE)
    {
        for (uint32_t i = 0; i < imageViews.size(); i++)
        {
            vkDestroyImageView(device, imageViews[i], nullptr);
        }
        images.clear();
        imageViews.clear();
        vkDestroySwapchainKHR(device, swapchain, nullptr);
    }
}
