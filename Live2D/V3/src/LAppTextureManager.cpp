/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "LAppTextureManager.hpp"
#include "GraphicsBackend.hpp"
#include "Log.hpp"
#include "FileUtils.hpp"
#ifdef CSM_TARGET_VULKAN
#include "VulkanManager.hpp"
#endif
#define STBI_NO_STDIO
#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
namespace Live2D {
namespace V3 {

using namespace Live2D::Cubism::Framework;

LAppTextureManager::LAppTextureManager()
#ifdef CSM_TARGET_VULKAN
    :
    mSequenceId(0),
    mMipLevels(0)
#endif
{
}

LAppTextureManager::~LAppTextureManager() {
    ReleaseTextures();
}

#ifdef CSM_TARGET_VULKAN
void LAppTextureManager::CopyBufferToImage(VkCommandBuffer commandBuffer, const VkBuffer& buffer,
                                           VkImage image, uint32_t width, uint32_t height) {
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(
        commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void LAppTextureManager::GenerateMipmaps(CubismImageVulkan image, uint32_t texWidth,
                                         uint32_t texHeight, uint32_t mipLevels) {
    VkCommandBuffer commandBuffer = VulkanManager::GetInstance()->BeginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image.GetImage();
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0,
                             0,
                             nullptr,
                             0,
                             nullptr,
                             1,
                             &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {
            mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(commandBuffer,
                       image.GetImage(),
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       image.GetImage(),
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1,
                       &blit,
                       VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0,
                             0,
                             nullptr,
                             0,
                             nullptr,
                             1,
                             &barrier);

        if (mipWidth > 1)
            mipWidth /= 2;
        if (mipHeight > 1)
            mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0,
                         0,
                         nullptr,
                         0,
                         nullptr,
                         1,
                         &barrier);

    VulkanManager::GetInstance()->SubmitCommand(commandBuffer);
}

LAppTextureManager::TextureInfo* LAppTextureManager::CreateTextureFromPngFile(
    std::string fileName, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
    VkMemoryPropertyFlags imageProperties, Csm::csmFloat32 anisotropy) {
    VkDevice device = VulkanManager::GetInstance()->GetDevice();
    VkPhysicalDevice physicalDevice = VulkanManager::GetInstance()->GetPhysicalDevice();

    //search loaded texture already.
    for (Csm::csmUint32 i = 0; i < mTextureInfos.GetSize(); i++) {
        if (mTextureInfos[i]->fileName == fileName) {
            return mTextureInfos[i];
        }
    }

    int width, height, channels;
    unsigned int size;
    unsigned char* png;
    unsigned char* address;

    address = Common::FileUtils::LoadFileAsBytes(fileName, &size);
    if (address == NULL) {
        LOGE("couldn't load texture file: %s", fileName.c_str());
        return NULL;
    }

    // png情報を取得する
    png = stbi_load_from_memory(
        address, static_cast<int>(size), &width, &height, &channels, STBI_rgb_alpha);
    if (!png) {
        LOGE("couldn't decode texture image: %s", fileName.c_str());
        Common::FileUtils::ReleaseBytes(address);
        return NULL;
    }
    {
#ifdef PREMULTIPLIED_ALPHA_ENABLE
        unsigned int* fourBytes = reinterpret_cast<unsigned int*>(png);
        for (int i = 0; i < width * height; i++) {
            unsigned char* p = png + i * 4;
            fourBytes[i] = LAppTextureManager_Common::Premultiply(p[0], p[1], p[2], p[3]);
        }
#endif
    }

    VkDeviceSize imageSize = static_cast<VkDeviceSize>(width * height * 4);

    CubismBufferVulkan stagingBuffer;
    stagingBuffer.CreateBuffer(
        device,
        physicalDevice,
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    stagingBuffer.Map(device, imageSize);
    stagingBuffer.MemCpy(png, imageSize);
    stagingBuffer.UnMap(device);

    // 解放処理
    stbi_image_free(png);
    Common::FileUtils::ReleaseBytes(address);

    mMipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

    CubismImageVulkan textureImage;

    textureImage.CreateImage(
        device, physicalDevice, width, height, mMipLevels, format, tiling, usage);
    VkCommandBuffer commandBuffer = VulkanManager::GetInstance()->BeginSingleTimeCommands();
    textureImage.SetImageLayout(
        commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mMipLevels, VK_IMAGE_ASPECT_COLOR_BIT);
    CopyBufferToImage(
        commandBuffer, stagingBuffer.GetBuffer(), textureImage.GetImage(), width, height);
    VulkanManager::GetInstance()->SubmitCommand(commandBuffer);
    GenerateMipmaps(textureImage, width, height, mMipLevels);
    textureImage.CreateView(device, format, VK_IMAGE_ASPECT_COLOR_BIT, mMipLevels);
    textureImage.CreateSampler(device,
                               VK_SAMPLER_ADDRESS_MODE_REPEAT,
                               VK_FILTER_LINEAR,
                               VK_FILTER_LINEAR,
                               VK_SAMPLER_MIPMAP_MODE_LINEAR,
                               anisotropy,
                               mMipLevels);
    mTextures.PushBack(textureImage);

    LAppTextureManager::TextureInfo* textureInfo = new LAppTextureManager::TextureInfo();
    if (textureInfo != NULL) {
        mSequenceId++;
        textureInfo->fileName = fileName;
        textureInfo->width = width;
        textureInfo->height = height;
        textureInfo->id = mSequenceId;
        mTextureInfos.PushBack(textureInfo);
    }
    stagingBuffer.Destroy(device);
    return textureInfo;
}

bool LAppTextureManager::GetTexture(Csm::csmUint32 textureId, CubismImageVulkan& retTexture) const {
    for (Csm::csmUint32 i = 0; i < mTextureInfos.GetSize(); i++) {
        if (mTextureInfos[i]->id == textureId) {
            retTexture = mTextures[i];
            return true;
        }
    }

    return false;
}
#endif

#if defined(CSM_TARGET_WIN_GL) || defined(CSM_TARGET_LINUX_GL) || defined(CSM_TARGET_MAC_GL)
LAppTextureManager::TextureInfo* LAppTextureManager::CreateTextureFromPngFile(
    std::string fileName) {
    // search loaded texture already.
    for (Csm::csmUint32 i = 0; i < mTextureInfos.GetSize(); i++) {
        if (mTextureInfos[i]->fileName == fileName) {
            return mTextureInfos[i];
        }
    }

    GLuint textureId;
    int width, height, channels;
    unsigned int size;
    unsigned char* png;
    unsigned char* address;

    address = Common::FileUtils::LoadFileAsBytes(fileName, &size);
    if (address == NULL) {
        LOGE("couldn't load texture file: %s", fileName.c_str());
        return NULL;
    }

    // png情報を取得する
    png = stbi_load_from_memory(
        address, static_cast<int>(size), &width, &height, &channels, STBI_rgb_alpha);
    if (!png) {
        LOGE("couldn't decode texture image: %s", fileName.c_str());
        Common::FileUtils::ReleaseBytes(address);
        return NULL;
    }
    {

#ifdef PREMULTIPLIED_ALPHA_ENABLE
        unsigned int* fourBytes = reinterpret_cast<unsigned int*>(png);
        for (int i = 0; i < width * height; i++) {
            unsigned char* p = png + i * 4;
            fourBytes[i] = Premultiply(p[0], p[1], p[2], p[3]);
        }
#endif
    }

    // OpenGL用のテクスチャを生成する
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, png);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    // 解放処理
    stbi_image_free(png);
    Common::FileUtils::ReleaseBytes(address);

    LAppTextureManager::TextureInfo* textureInfo = new LAppTextureManager::TextureInfo();

    textureInfo->fileName = fileName;
    textureInfo->width = width;
    textureInfo->height = height;
    textureInfo->id = textureId;

    mTextureInfos.PushBack(textureInfo);


    return textureInfo;
}
#endif

LAppTextureManager::TextureInfo* LAppTextureManager::GetTextureInfoById(
    unsigned int textureId) const {
    for (Csm::csmUint32 i = 0; i < mTextureInfos.GetSize(); i++) {
        if (mTextureInfos[i]->id == textureId) {
            return mTextureInfos[i];
        }
    }

    return NULL;
}


void LAppTextureManager::ReleaseTextures() {
#ifdef CSM_TARGET_VULKAN
    if (Common::Backend::IsVulkan()) {
        VkDevice device = VulkanManager::GetInstance()->GetDevice();

        for (Csm::csmUint32 i = 0; i < mTextureInfos.GetSize(); i++) {
            mTextures[i].Destroy(device);
            delete mTextureInfos[i];
        }

        mTextureInfos.Clear();
        return;
    }
#endif
#if defined(CSM_TARGET_WIN_GL) || defined(CSM_TARGET_LINUX_GL) || defined(CSM_TARGET_MAC_GL)
    if (Common::Backend::IsOpenGL()) {
        for (Csm::csmUint32 i = 0; i < mTextureInfos.GetSize(); i++) {
            glDeleteTextures(1, &(mTextureInfos[i]->id));
            delete mTextureInfos[i];
        }

        mTextureInfos.Clear();
    }
#endif
}

void LAppTextureManager::ReleaseTexture(Csm::csmUint32 textureId) {
#ifdef CSM_TARGET_VULKAN
    if (Common::Backend::IsVulkan()) {
        VkDevice device = VulkanManager::GetInstance()->GetDevice();

        for (Csm::csmUint32 i = 0; i < mTextureInfos.GetSize(); i++) {
            if (mTextureInfos[i]->id != textureId) {
                continue;
            }
            // ID一致
            // info除去
            delete mTextureInfos[i];

            // 実体除去
            VkDevice device = VulkanManager::GetInstance()->GetDevice();
            mTextures[i].Destroy(device);

            mTextureInfos.Remove(i);
            mTextures.Remove(i);
            break;
        }
        if (mTextureInfos.GetSize() == 0) {
            mTextureInfos.Clear();
        }
        if (mTextures.GetSize() == 0) {
            mTextures.Clear();
        }
        return;
    }
#endif
#if defined(CSM_TARGET_WIN_GL) || defined(CSM_TARGET_LINUX_GL) || defined(CSM_TARGET_MAC_GL)
    if (Common::Backend::IsOpenGL()) {
        for (Csm::csmUint32 i = 0; i < mTextureInfos.GetSize(); i++) {
            if (mTextureInfos[i]->id != textureId) {
                continue;
            }
            glDeleteTextures(1, &(mTextureInfos[i]->id));
            delete mTextureInfos[i];
            mTextureInfos.Remove(i);
            break;
        }
    }
#endif
}

void LAppTextureManager::ReleaseTexture(std::string fileName) {
#ifdef CSM_TARGET_VULKAN
    if (Common::Backend::IsVulkan()) {
        for (Csm::csmUint32 i = 0; i < mTextureInfos.GetSize(); i++) {
            if (mTextureInfos[i]->fileName == fileName) {
                // info除去
                delete mTextureInfos[i];

                // 実体除去
                if (mTextureInfos[i] != VK_NULL_HANDLE) {
                    VkDevice device = VulkanManager::GetInstance()->GetDevice();
                    mTextures[i].Destroy(device);
                    mTextureInfos[i] = VK_NULL_HANDLE;
                }
                mTextureInfos.Remove(i);
                mTextureInfos.Remove(i);
                break;
            }
        }
        if (mTextureInfos.GetSize() == 0) {
            mTextureInfos.Clear();
        }
        if (mTextureInfos.GetSize() == 0) {
            mTextureInfos.Clear();
        }
        return;
    }
#endif
#if defined(CSM_TARGET_WIN_GL) || defined(CSM_TARGET_LINUX_GL) || defined(CSM_TARGET_MAC_GL)
    if (Common::Backend::IsOpenGL()) {
        for (Csm::csmUint32 i = 0; i < mTextureInfos.GetSize(); i++) {
            if (mTextureInfos[i]->fileName == fileName) {
                glDeleteTextures(1, &(mTextureInfos[i]->id));
                delete mTextureInfos[i];
                mTextureInfos.Remove(i);
                break;
            }
        }
    }
#endif
}
}   // namespace V3
}   // namespace Live2D