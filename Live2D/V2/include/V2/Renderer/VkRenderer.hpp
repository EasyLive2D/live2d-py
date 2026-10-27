#pragma once

#include "IRenderer.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include <vulkan/vulkan.h>

namespace Live2D {
namespace V2 {

class Moc;
class ClippingManagerVulkan;

/**
 * VkRenderer — V2 的 Vulkan 渲染后端（与 GLRenderer 平行、接口一致）
 *
 * 与 GLRenderer 的对应关系：
 *  - 2 套 GLSL 1.20 program → 2 套 GLSL 450 shader（normal / mask），各含 3 种
 *    固定管线混合（NORMAL/SCREEN/MULTIPLY）→ 共 6 条 graphics pipeline
 *  - per-draw VBO/EBO → 帧级 host-visible 环形 arena（VBO/IBO/UBO），帧首复位
 *  - uniform 上传 → UBO ring + VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
 *  - FBO 裁剪遮罩 → 256x256 offscreen image（mask pass 动态渲染 → barrier → 主 pass 采样）
 *  - glViewport → 负高度 viewport（保持 GL NDC/y 语义，矩阵无需翻转）
 *
 * 帧流程（帧解耦，由外部 vkBeginFrame/vkEndFrame 驱动，见 VulkanManager）：
 *  preDraw（mask 脏时：挂起帧主区间 → 录制 mask 区间 → 恢复）→ draw（纯录制进帧主区间）
 */
class VkRenderer : public IRenderer {
public:
    static constexpr int COLOR_COMPOSITION_NORMAL = 0;
    static constexpr int COLOR_COMPOSITION_SCREEN = 1;
    static constexpr int COLOR_COMPOSITION_MULTIPLY = 2;

    VkRenderer(Moc* moc, int textureCount);
    ~VkRenderer() override;

    VkRenderer* GetVkRenderer() override { return this; }

    bool IsVulkan() const override { return true; }

    void PreDraw(Moc* moc) override;
    void Draw(Moc* moc) override;
    void SetMaskDirty() override { mMaskDirty = true; }

    void SetMatrix(const float m[16]) override;
    void Resize(int w, int h) override;

    /// 绑定外部创建的纹理（VkUtil::CreateTextureFromPixels 产物），资源所有权移交本类并在析构释放
    void SetTexture(int no, VkImage image, VkDeviceMemory memory, VkImageView view,
                    VkSampler sampler);

    // ---- 以下为渲染器内部 API（ClippingManager / draw 使用），不属于 IRenderer 接口 ----
    /// 按 Moc 数据面提交单个 drawable（mask 模式下跳过 clip 状态切换）
    void DrawDrawable(Moc* moc, int index);
    void SetClipMatrix(const float m[16]);
    void SetClipChannel(int channel) { mClipChannel = channel; }
    void SetClipDrawMode(bool on) { mClipDrawMode = on; }
    void SetClipMaskMode(bool on) { mClipMaskMode = on; }
    void SetCulling(bool cull) { mCulling = cull; }

private:
    void DrawTexture(int texNo, const std::array<float, 4>& screenColor, const uint16_t* indices,
                     int indexCount, const float* vertices, int vertexFloatCount, const float* uvs,
                     int uvFloatCount, float opacity, int compositionType,
                     const std::array<float, 4>& multiplyColor);

    // std140 与 shader UBO 块布局一一对应
    struct UBO {
        float mvp[16];            // 0
        float clipMatrix[16];     // 64
        float baseColor[4];       // 128
        float screenColor[4];     // 144
        float multiplyColor[4];   // 160
        float channelFlag[4];     // 176
        int32_t maskFlag;         // 192
    };
    static_assert(sizeof(UBO) == 196, "UBO layout must match std140 block in v2_*.vert/frag");

    struct TextureRes {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;
        bool valid = false;
    };

    void CreateLayoutsAndPipelines();
    VkShaderModule LoadShaderModule(const char* path);
    VkPipeline CreatePipeline(VkShaderModule vert, VkShaderModule frag,
                              const VkPipelineColorBlendAttachmentState& blend);
    void CreateMaskTarget();
    void ImageBarrier(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout,
                      VkImageLayout newLayout);

    VkDevice mDevice = VK_NULL_HANDLE;
    VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
    VkFormat mColorFormat =
        VK_FORMAT_R8G8B8A8_UNORM;   // swapchain/mask 通用颜色格式（管线 rendering info）

    std::array<float, 16> mMatrix4x4{};
    std::array<float, 16> mClipMatrix{};
    bool mCulling = true;
    bool mClipMaskMode = false;   // Mask RENDER pass（写 mask image）
    bool mClipDrawMode = false;   // Clipped DRAW pass（采样 mask image）
    int mClipChannel = 0;

    // 管线/描述符
    VkDescriptorSetLayout mDescLayout = VK_NULL_HANDLE;
    VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
    VkPipeline mPipelines[6] = {};   // [0..2]=normal×blend, [3..5]=mask×blend
    VkDescriptorPool mDescPool = VK_NULL_HANDLE;
    std::vector<std::vector<VkDescriptorSet>> mFrameDescSets;   // 每纹理一个 set

    // 纹理资源
    std::vector<TextureRes> mTextures;

    // 帧级 arena（host-visible，SubmitCommand 每次等待完成，故单帧安全复用）
    struct Memory {
        VkDeviceMemory memory;
        VkBuffer buffer;
        void* mapped;
    };

    VkDeviceSize mVbOffset;
    std::vector<Memory> mVbs;
    VkDeviceSize mIbOffset;
    std::vector<Memory> mIbs;
    std::vector<Memory> mUbs;

    uint32_t mUboIndex = 0;
    uint32_t mUboStride = 256;
    // arena 容量按模型 drawable 数分配（见构造函数），帧内溢出时丢弃该次 draw
    VkDeviceSize mVbCapacity = 0;
    VkDeviceSize mIbCapacity = 0;
    uint32_t mUboSlots = 0;

    // 裁剪遮罩 offscreen
    VkImage mMaskImage = VK_NULL_HANDLE;
    VkDeviceMemory mMaskMemory = VK_NULL_HANDLE;
    VkImageView mMaskView = VK_NULL_HANDLE;
    VkSampler mMaskSampler = VK_NULL_HANDLE;
    static constexpr uint32_t MASK_SIZE = 256;

    VkCommandBuffer mCurrentCmd = VK_NULL_HANDLE;   // 当前录制目标
    bool mMaskDirty = true;   // mask 需重渲（首次为 true，此后由 setMaskDirty 标记）

    std::shared_ptr<ClippingManagerVulkan> mClipManager;

    const uint32_t mMaxFramesInFlight;
    uint32_t mCurrentFrame;
};

}   // namespace V2
}   // namespace Live2D
