/**
 * IRenderer — V2 渲染器抽象接口
 * GLRenderer（OpenGL/EGL）与 VkRenderer（Vulkan）共同实现此接口，
 * 与 V3 的 CubismRenderer_OpenGLES2 / CubismRenderer_Vulkan 并列结构一致。
 *
 * 渲染数据全部经 Moc 只读数据面获取（getRenderSequence + getDrawable*），
 * 逐 drawable 的提交细节为各渲染器内部实现，不在本接口暴露。
 */

#pragma once

namespace Live2D {
namespace V2 {

class Moc;


class VkRenderer;
class GLRenderer;
class IRenderer {
public:
    virtual ~IRenderer() = default;

    /// 是否为 Vulkan 后端
    virtual bool IsVulkan() const = 0;

    /**
     * @brief 绘制前的处理（裁剪遮罩 pass 等）
     */
    virtual void PreDraw(Moc* moc) = 0;

    /**
     * @brief 绘制所有可见 drawable（按 Moc::getRenderSequence 顺序）
     */
    virtual void Draw(Moc* moc) = 0;

    /**
     * @brief 标记裁剪遮罩内容已变化需重渲（仅 Vulkan 后端使用，GL 默认每帧重渲）
     */
    virtual void SetMaskDirty() {}

    virtual void SetMatrix(const float m[16]) = 0;
    virtual void Resize(int w, int h) = 0;

    virtual VkRenderer* GetVkRenderer() { return nullptr; }
    virtual GLRenderer* GetGLRenderer() { return nullptr; }
};

}   // namespace V2
}   // namespace Live2D
