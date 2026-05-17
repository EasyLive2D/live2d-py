#pragma once

#include <GL/glew.h>
#include <vector>
#include <array>
#include <cstdint>
#include <string>

namespace live2d {

class DrawParamOpenGL {
public:
    static constexpr int NORMAL_SHADER = 0;
    static constexpr int MASK_SHADER = 1;
    static constexpr int COLOR_COMPOSITION_NORMAL = 0;
    static constexpr int COLOR_COMPOSITION_SCREEN = 1;
    static constexpr int COLOR_COMPOSITION_MULTIPLY = 2;

    DrawParamOpenGL();
    ~DrawParamOpenGL();

    void setupDraw();
    void endDraw();

    void setClipBufPre_clipContextForDraw(void* ctx) { mClipDrawCtx = ctx; }
    void setClipBufPre_clipContextForMask(void* ctx) { mClipMaskCtx = ctx; }
    void* mClipMaskCtx = nullptr;   // Mask RENDER pass (writes to FBO)
    void* mClipDrawCtx = nullptr;   // Clipped DRAW pass (uses mask)
    int mClipChannel = 0;
    void setCulling(bool cull) { mCulling = cull; }
    static void clearBuffer(float r, float g, float b, float a);

    void drawTexture(int texNo, const std::array<float, 4>& screenColor,
                     const std::vector<int16_t>& indices,
                     const std::vector<float>& vertices,
                     const std::vector<float>& uvs,
                     float opacity, int compositionType,
                     const std::array<float, 4>& multiplyColor);

    int createFramebuffer();
    void bindFramebuffer(int fb);
    void setTexture(int no, GLuint texId);
    GLuint getTexture(int no) const;

    void setMatrix(const float m[16]);
    void setClipMatrix(const float m[16]);

    // Framebuffers for clipping masks
    GLuint mFramebuffer = 0;
    GLuint mFramebufferTexture = 0;

private:
    void initShader();
    GLuint compileShader(GLenum type, const char* src);
    GLuint createVBO(const std::vector<float>& data, int loc, int size);
    GLuint createEBO(const std::vector<int16_t>& data);

    GLuint mShaderNormal = 0, mShaderMask = 0;
    GLuint mPosVBO = 0, mUVVBO = 0, mEBO = 0;
    std::array<float, 16> mMatrix4x4{};
    std::array<float, 16> mClipMatrix{};
    bool mCulling = true;

    void lazyInit();
    std::vector<GLuint> mTextures;
    GLuint mCurrentFBO = 0;
    bool mInited = false;
    float mBaseRed = 1.0f, mBaseGreen = 1.0f, mBaseBlue = 1.0f, mBaseAlpha = 1.0f;
};

} // namespace live2d
