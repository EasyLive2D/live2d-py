#pragma once

#include "ClippingManagerOpenGL.hpp"
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

class ModelContext;

class GLRenderer
{
public:
    static constexpr int NORMAL_SHADER = 0;
    static constexpr int MASK_SHADER = 1;
    static constexpr int COLOR_COMPOSITION_NORMAL = 0;
    static constexpr int COLOR_COMPOSITION_SCREEN = 1;
    static constexpr int COLOR_COMPOSITION_MULTIPLY = 2;

    GLRenderer(ModelContext* modelContext, int textureCount);
    ~GLRenderer();

    void setupDraw(ModelContext* modelContext);
    void endDraw();

    void preDraw(ModelContext* modelContext);
    void draw(ModelContext* modelContext);

    void setClipBufPre_clipContextForDraw(void* ctx) { mClipDrawCtx = ctx; }
    void setClipBufPre_clipContextForMask(void* ctx) { mClipMaskCtx = ctx; }
    void* mClipMaskCtx = nullptr;   // Mask RENDER pass (writes to FBO)
    void* mClipDrawCtx = nullptr;   // Clipped DRAW pass (uses mask)
    int mClipChannel = 0;
    void setCulling(bool cull) { mCulling = cull; }
    static void clearBuffer(float r, float g, float b, float a);

    void drawTexture(int texNo, const std::array<float, 4>& screenColor,
                     const std::vector<int16_t>& indices, const std::vector<float>& vertices,
                     const std::vector<float>& uvs, float opacity, int compositionType,
                     const std::array<float, 4>& multiplyColor);

    int createFramebuffer();
    void bindFramebuffer(int fb = -1);
    void setTexture(int no, GLuint texId);
    GLuint getTexture(int no) const;

    void setMatrix(const float m[16]);
    void setClipMatrix(const float m[16]);

    void resize(int w, int h);

private:
    void init(ModelContext* modelContext);

    void initShaders();
    GLuint compileShader(GLenum type, const char* src);
    GLuint createVBO(const std::vector<float>& data, int loc, int size);
    GLuint createEBO(const std::vector<int16_t>& data);

    GLuint mShaderNormal = 0, mShaderMask = 0;
    // Uniform locations resolved once per program in initShaders()
    // (avoid per-draw glGetUniformLocation driver lookups)
    struct UniformLocs
    {
        GLint normMvp = -1, normMaskFlag = -1, normBaseColor = -1, normChannelFlag = -1,
              normScreenColor = -1, normMultiplyColor = -1, normTexture0 = -1;
        GLint maskMvp = -1, maskClipMatrix = -1, maskBaseColor = -1, maskChannelFlag = -1,
              maskScreenColor = -1, maskMultiplyColor = -1, maskTexture0 = -1, maskTexture1 = -1;
    } mUniforms;
    GLuint mPosVBO = 0, mUVVBO = 0, mEBO = 0;
    std::array<float, 16> mMatrix4x4{};
    std::array<float, 16> mClipMatrix{};
    bool mCulling = true;

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