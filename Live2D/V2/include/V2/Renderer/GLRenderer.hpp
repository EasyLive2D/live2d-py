#pragma once

#include "ClippingManagerOpenGL.hpp"
#include "Drawable.hpp"
#include "IRenderer.hpp"
#ifdef __ANDROID__
#include <GLES/gl.h>
#else
#include <GL/glew.h>
#endif
#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace Live2D {
namespace V2 {

class Moc;

class GLRenderer : public IRenderer {
public:
    static constexpr int NORMAL_SHADER = 0;
    static constexpr int MASK_SHADER = 1;
    static constexpr int COLOR_COMPOSITION_NORMAL = 0;
    static constexpr int COLOR_COMPOSITION_SCREEN = 1;
    static constexpr int COLOR_COMPOSITION_MULTIPLY = 2;

    GLRenderer(Moc* moc, int textureCount);
    ~GLRenderer() override;

    GLRenderer* GetGLRenderer() override { return this; }

    void SetupDraw(Moc* moc);
    void EndDraw();

    void PreDraw(Moc* moc) override;
    void Draw(Moc* moc) override;

    bool IsVulkan() const override { return false; }
    static void ClearBuffer(float r, float g, float b, float a);

    int CreateFramebuffer();
    void BindFramebuffer(int fb = -1);
    void SetTexture(int no, GLuint texId);
    GLuint GetTexture(int no) const;

    void SetMatrix(const float m[16]) override;

    void Resize(int w, int h) override;

    // ---- 以下为渲染器内部 API（ClippingManager / draw 使用），不属于 IRenderer 接口 ----
    /// 按 Moc 数据面提交单个 drawable（mask 模式下跳过 clip 状态切换）
    void DrawDrawable(Moc* moc, int index);
    void SetClipDrawMode(bool on) { mClipDrawMode = on; }
    void SetClipMaskMode(bool on) { mClipMaskMode = on; }
    void SetClipChannel(int channel) { mClipChannel = channel; }
    void SetCulling(bool cull) { mCulling = cull; }
    void SetClipMatrix(const float m[16]);

private:
    void Init(Moc* moc);

    void DrawTexture(int drawableIndex, int texNo, const std::array<float, 4>& screenColor, const uint16_t* indices,
                     int indexCount, const float* vertices, int vertexFloatCount, const float* uvs,
                     int uvFloatCount, float opacity, int compositionType,
                     const std::array<float, 4>& multiplyColor);

    void InitShaders();
    GLuint CompileShader(GLenum type, const char* src);
    GLuint CreateVBO(const std::vector<float>& data, int loc, int size);
    GLuint CreateEBO(const std::vector<int16_t>& data);

    GLuint mShaderNormal = 0, mShaderMask = 0;
    // Uniform locations resolved once per program in InitShaders()
    // (avoid per-draw glGetUniformLocation driver lookups)
    struct UniformLocs {
        GLint normMvp = -1, normMaskFlag = -1, normBaseColor = -1, normChannelFlag = -1,
              normScreenColor = -1, normMultiplyColor = -1, normTexture0 = -1;
        GLint maskMvp = -1, maskClipMatrix = -1, maskBaseColor = -1, maskChannelFlag = -1,
              maskScreenColor = -1, maskMultiplyColor = -1, maskTexture0 = -1, maskTexture1 = -1;
    } mUniforms;

    struct DrawObject {
        GLuint vao;
        GLuint vbo;
        GLuint uvbo;
        GLuint ebo;
    };

    std::vector<DrawObject> mDrawObjects;
    std::array<float, 16> mMatrix4x4{};
    std::array<float, 16> mClipMatrix{};
    bool mCulling = true;
    bool mClipMaskMode = false;   // Mask RENDER pass (writes to FBO)
    bool mClipDrawMode = false;   // Clipped DRAW pass (uses mask)
    int mClipChannel = 0;

    std::vector<GLuint> mTextures;
    GLuint mCurrentFBO = 0;
    float mBaseRed = 1.0f, mBaseGreen = 1.0f, mBaseBlue = 1.0f, mBaseAlpha = 1.0f;
    // Framebuffers for clipping masks
    GLuint mFramebuffer = 0;
    GLuint mFramebufferTexture = 0;
    GLuint mCurrentProgram = 0;

    std::shared_ptr<ClippingManagerOpenGL> mClipManager;
};

}   // namespace V2
}   // namespace Live2D