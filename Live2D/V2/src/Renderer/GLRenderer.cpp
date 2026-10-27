#include "GLRenderer.hpp"
#include "GL/glew.h"
#include "Log.hpp"
#include "ClippingManagerOpenGL.hpp"
#include "ClipContext.hpp"
#include "Moc.hpp"
#include "FileUtils.hpp"
#include <cstring>
#include <string>
#ifdef __ANDROID__
#include <GLES3/gl3.h>
#endif


namespace Live2D {
namespace V2 {

using namespace Live2D::Common::Log;

// GLSL shader 源文件按 V3 Framework 的方式管理：
//   源文件位于 Live2D/V2/shaders/opengl/，构建时拷到 package/live2d/FrameworkShaders，
//   运行时经 Common::FileUtils 的 SHADER_DIR 重定向读取。
// 文件名（带 V2 前缀，避免与 V3 的 VertShaderSrc*/FragShaderSrc* 冲突）：
//   V2NormalGL.vert / V2NormalGL.frag / V2MaskGL.vert / V2MaskGL.frag
static std::string LoadShaderSource(const char* fileName) {
    const std::string path = std::string("FrameworkShaders/") + fileName;
    unsigned int size = 0;
    unsigned char* bytes = Live2D::Common::FileUtils::LoadFileAsBytes(path, &size);
    if (bytes == nullptr || size == 0) {
        LOGE("Failed to load shader file: %s", path.c_str());
        if (bytes != nullptr)
            Live2D::Common::FileUtils::ReleaseBytes(bytes);
        return std::string();
    }
    std::string source(reinterpret_cast<const char*>(bytes), size);
    Live2D::Common::FileUtils::ReleaseBytes(bytes);
    return source;
}

GLRenderer::GLRenderer(Moc* moc, int textureCount) :
    mShaderNormal(0), mShaderMask(0), mFramebuffer(0), mFramebufferTexture(0), mCulling(false) {
    mTextures.resize(textureCount, 0);
    Init(moc);
}

void GLRenderer::Init(Moc* moc) {
    InitShaders();

    mClipManager = std::make_unique<ClippingManagerOpenGL>(*this);
    mClipManager->Init(moc);

    const int drawableCount = moc->GetDrawableCount();
    mDrawObjects.resize(drawableCount, {});
    GLuint vao, vbo, uvbo, ebo, vertexFloatCount, indexCount;

    for (int i = 0; i < drawableCount; i ++) {
        vertexFloatCount = moc->GetDrawableVertexCount(i) * 2;
        indexCount = moc->GetDrawableVertexIndexCount(i);
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        // Position VBO (location 0)
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertexFloatCount * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);

        // UV VBO (location 1)
        glGenBuffers(1, &uvbo);
        glBindBuffer(GL_ARRAY_BUFFER, uvbo);
        glBufferData(GL_ARRAY_BUFFER, vertexFloatCount * sizeof(float), moc->GetDrawableVertexUvs(i), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(1);

        // EBO
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(uint16_t), moc->GetDrawableVertexIndices(i), GL_STATIC_DRAW);
        
        mDrawObjects[i].vao = vao;
        mDrawObjects[i].vbo = vbo;
        mDrawObjects[i].uvbo = uvbo;
        mDrawObjects[i].ebo = ebo;

        glBindVertexArray(0);
    }
}

GLRenderer::~GLRenderer() {
    if (mShaderNormal)
        glDeleteProgram(mShaderNormal);
    if (mShaderMask)
        glDeleteProgram(mShaderMask);
    if (mFramebuffer)
        glDeleteFramebuffers(1, &mFramebuffer);
    if (mFramebufferTexture)
        glDeleteTextures(1, &mFramebufferTexture);
    for (auto& it : mDrawObjects) {
        glDeleteVertexArrays(1, &it.vao);
        glDeleteBuffers(1, &it.vbo);
        glDeleteBuffers(1, &it.uvbo);
        glDeleteBuffers(1, &it.ebo);
    }
    for (auto t : mTextures)
        if (t)
            glDeleteTextures(1, &t);
}

GLuint GLRenderer::CompileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char buf[512];
        glGetShaderInfoLog(shader, 512, nullptr, buf);
        LOGE("Shader compile error (%s): %s\n",
             type == GL_VERTEX_SHADER ? "vertex" : "fragment",
             buf);
    }
    return shader;
}

void GLRenderer::InitShaders() {
    // Normal program: V2NormalGL.vert + V2NormalGL.frag
    const std::string normVertSrc = LoadShaderSource("V2NormalGL.vert");
    const std::string normFragSrc = LoadShaderSource("V2NormalGL.frag");
    GLuint vsNorm = CompileShader(GL_VERTEX_SHADER, normVertSrc.c_str());
    GLuint fsNorm = CompileShader(GL_FRAGMENT_SHADER, normFragSrc.c_str());
    mShaderNormal = glCreateProgram();
    glAttachShader(mShaderNormal, vsNorm);
    glAttachShader(mShaderNormal, fsNorm);
    // Explicitly bind attribute locations before linking (match Python order)
    glBindAttribLocation(mShaderNormal, 0, "a_position");
    glBindAttribLocation(mShaderNormal, 1, "a_texCoord");
    glLinkProgram(mShaderNormal);
    // Check link status
    GLint linked;
    glGetProgramiv(mShaderNormal, GL_LINK_STATUS, &linked);
    if (!linked) {
        char buf[512];
        glGetProgramInfoLog(mShaderNormal, 512, nullptr, buf);
        LOGE("Normal program link error: %s\n", buf);
    }
    glDeleteShader(vsNorm);
    glDeleteShader(fsNorm);

    // Mask program: V2MaskGL.vert + V2MaskGL.frag
    const std::string maskVertSrc = LoadShaderSource("V2MaskGL.vert");
    const std::string maskFragSrc = LoadShaderSource("V2MaskGL.frag");
    GLuint vsMask = CompileShader(GL_VERTEX_SHADER, maskVertSrc.c_str());
    GLuint fsMask = CompileShader(GL_FRAGMENT_SHADER, maskFragSrc.c_str());
    mShaderMask = glCreateProgram();
    glAttachShader(mShaderMask, vsMask);
    glAttachShader(mShaderMask, fsMask);
    glBindAttribLocation(mShaderMask, 0, "a_position");
    glBindAttribLocation(mShaderMask, 1, "a_texCoord");
    glLinkProgram(mShaderMask);
    glGetProgramiv(mShaderMask, GL_LINK_STATUS, &linked);
    if (!linked) {
        char buf[512];
        glGetProgramInfoLog(mShaderMask, 512, nullptr, buf);
        LOGE("Mask program link error: %s\n", buf);
    }
    glDeleteShader(vsMask);
    glDeleteShader(fsMask);

    // Resolve uniform locations once per program (per-draw lookups are pure
    // driver round-trips; locations are stable for the program lifetime)
    mUniforms.normMvp = glGetUniformLocation(mShaderNormal, "u_mvpMatrix");
    mUniforms.normMaskFlag = glGetUniformLocation(mShaderNormal, "u_maskFlag");
    mUniforms.normBaseColor = glGetUniformLocation(mShaderNormal, "u_baseColor");
    mUniforms.normChannelFlag = glGetUniformLocation(mShaderNormal, "u_channelFlag");
    mUniforms.normScreenColor = glGetUniformLocation(mShaderNormal, "u_screenColor");
    mUniforms.normMultiplyColor = glGetUniformLocation(mShaderNormal, "u_multiplyColor");
    mUniforms.normTexture0 = glGetUniformLocation(mShaderNormal, "s_texture0");

    mUniforms.maskMvp = glGetUniformLocation(mShaderMask, "u_mvpMatrix");
    mUniforms.maskClipMatrix = glGetUniformLocation(mShaderMask, "u_clipMatrix");
    mUniforms.maskBaseColor = glGetUniformLocation(mShaderMask, "u_baseColor");
    mUniforms.maskChannelFlag = glGetUniformLocation(mShaderMask, "u_channelFlag");
    mUniforms.maskScreenColor = glGetUniformLocation(mShaderMask, "u_screenColor");
    mUniforms.maskMultiplyColor = glGetUniformLocation(mShaderMask, "u_multiplyColor");
    mUniforms.maskTexture0 = glGetUniformLocation(mShaderMask, "s_texture0");
    mUniforms.maskTexture1 = glGetUniformLocation(mShaderMask, "s_texture1");
}

GLuint GLRenderer::CreateVBO(const std::vector<float>& data, int loc, int size) {
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(loc, size, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(loc);
    return vbo;
}

GLuint GLRenderer::CreateEBO(const std::vector<int16_t>& data) {
    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER, data.size() * sizeof(int16_t), data.data(), GL_STATIC_DRAW);
    return ebo;
}

void GLRenderer::ClearBuffer(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void GLRenderer::SetupDraw(Moc* moc) {
    glGetIntegerv(GL_CURRENT_PROGRAM, reinterpret_cast<GLint*>(&mCurrentProgram));
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, reinterpret_cast<GLint*>(&mCurrentFBO));
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DEPTH_TEST);
    glFrontFace(GL_CW);
    glEnable(GL_BLEND);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    // glBindBuffer(GL_ARRAY_BUFFER, 0);
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void GLRenderer::EndDraw() {
    glBindVertexArray(0);
    // glDisableVertexAttribArray(0);
    // glDisableVertexAttribArray(1);
    // glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    glBindFramebuffer(GL_FRAMEBUFFER, mCurrentFBO);
    // When changing model: v2cpp => v3,
    // v3 may get save the wrong program id to `lastProgramId`
    // and produce an silent gl error when restore `lastProgramId`.
    // This error will be checked and raised in v2.
    // Thus, re-bind program to `0`
    glUseProgram(mCurrentProgram);
}

void GLRenderer::SetMatrix(const float m[16]) {
    std::memcpy(mMatrix4x4.data(), m, 16 * sizeof(float));
}

void GLRenderer::SetClipMatrix(const float m[16]) {
    std::memcpy(mClipMatrix.data(), m, 16 * sizeof(float));
}

int GLRenderer::CreateFramebuffer() {
    if (mFramebuffer == 0)
        glGenFramebuffers(1, &mFramebuffer);
    if (mFramebufferTexture == 0)
        glGenTextures(1, &mFramebufferTexture);

    glBindTexture(GL_TEXTURE_2D, mFramebufferTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mFramebufferTexture, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, mCurrentFBO);

    return (int)mFramebuffer;
}

void GLRenderer::BindFramebuffer(int fb) {
    if (fb == -1)
        fb = mFramebuffer;
    glBindFramebuffer(GL_FRAMEBUFFER, fb ? fb : mCurrentFBO);
}

void GLRenderer::SetTexture(int no, GLuint texId) {
    if (no >= (int)mTextures.size())
        mTextures.resize(no + 1, 0);
    mTextures[no] = texId;
}

GLuint GLRenderer::GetTexture(int no) const {
    if (no >= 0 && no < (int)mTextures.size())
        return mTextures[no];
    return 0;
}

void GLRenderer::DrawTexture(int drawbaleIndex, int texNo, const std::array<float, 4>& screenColor,
                             const uint16_t* indices, int indexCount, const float* vertices,
                             int vertexFloatCount, const float* uvs, int uvFloatCount,
                             float opacity, int compositionType,
                             const std::array<float, 4>& multiplyColor) {
    if (opacity < 0.01f && !mClipMaskMode)
        return;

    float a_w = mBaseRed * opacity;
    float a2 = mBaseGreen * opacity;
    float a5 = mBaseBlue * opacity;
    float a7 = mBaseAlpha * opacity;

    if (mClipMaskMode) {
        // Path 1: Mask RENDER — use clip's matrixForMask as u_mvpMatrix
        glFrontFace(GL_CCW);
        glUseProgram(mShaderNormal);
        glUniformMatrix4fv(mUniforms.normMvp, 1, GL_FALSE, mClipMatrix.data());
        glUniform1i(mUniforms.normMaskFlag, 1);
        // u_baseColor = clip rect (full [-1,1] for full-FBO mask render)
        glUniform4f(mUniforms.normBaseColor, -1.0f, -1.0f, 1.0f, 1.0f);
        float chR = (mClipChannel == 0) ? 1.0f : 0, chG = (mClipChannel == 1) ? 1.0f : 0,
              chB = (mClipChannel == 2) ? 1.0f : 0, chA = (mClipChannel == 3) ? 1.0f : 0;
        // Channel color: R=ch0, G=ch1, B=ch2, A=ch3
        float cR = (mClipChannel == 0) ? 1.0f : 0, cG = (mClipChannel == 1) ? 1.0f : 0,
              cB = (mClipChannel == 2) ? 1.0f : 0, cA = (mClipChannel == 3) ? 1.0f : 0;
        glUniform4f(mUniforms.normChannelFlag, cR, cG, cB, cA);
        glUniform4f(mUniforms.normScreenColor, 0, 0, 0, 0);
        glUniform4f(mUniforms.normMultiplyColor, 1, 1, 1, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, GetTexture(texNo));
        glUniform1i(mUniforms.normTexture0, 1);
    } else if (mClipDrawMode) {
        // Path 2: Clipped DRAW — uses mask FBO
        glUseProgram(mShaderMask);
        glUniformMatrix4fv(mUniforms.maskMvp, 1, GL_FALSE, mMatrix4x4.data());
        glUniformMatrix4fv(mUniforms.maskClipMatrix, 1, GL_FALSE, mClipMatrix.data());
        glUniform4f(mUniforms.maskBaseColor, a_w, a2, a5, a7);
        glUniform4f(mUniforms.maskScreenColor,
                    screenColor[0],
                    screenColor[1],
                    screenColor[2],
                    screenColor[3]);
        glUniform4f(mUniforms.maskMultiplyColor,
                    multiplyColor[0],
                    multiplyColor[1],
                    multiplyColor[2],
                    multiplyColor[3]);
        float chR = (mClipChannel == 0) ? 1.0f : 0, chG = (mClipChannel == 1) ? 1.0f : 0,
              chB = (mClipChannel == 2) ? 1.0f : 0, chA = (mClipChannel == 3) ? 1.0f : 0;
        glUniform4f(mUniforms.maskChannelFlag, chR, chG, chB, chA);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, GetTexture(texNo));
        glUniform1i(mUniforms.maskTexture0, 1);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, mFramebufferTexture);
        glUniform1i(mUniforms.maskTexture1, 2);
        mClipDrawMode = false;
    } else {
        // Path 3: Normal draw
        glUseProgram(mShaderNormal);
        glUniformMatrix4fv(mUniforms.normMvp, 1, GL_FALSE, mMatrix4x4.data());
        glUniform1i(mUniforms.normMaskFlag, 0);
        glUniform4f(mUniforms.normBaseColor, a_w, a2, a5, a7);
        glUniform4f(mUniforms.normScreenColor,
                    screenColor[0],
                    screenColor[1],
                    screenColor[2],
                    screenColor[3]);
        glUniform4f(mUniforms.normMultiplyColor,
                    multiplyColor[0],
                    multiplyColor[1],
                    multiplyColor[2],
                    multiplyColor[3]);
        glUniform4f(mUniforms.normChannelFlag, 0, 0, 0, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, GetTexture(texNo));
        glUniform1i(mUniforms.normTexture0, 1);
    }

    // Culling (match v2 Python)
    if (mCulling) {
        glEnable(GL_CULL_FACE);
    } else {
        glDisable(GL_CULL_FACE);
    }

    glEnable(GL_BLEND);
    // Match v2 Python blend modes exactly
    // Python glBlendFuncSeparate(src_color, src_factor, dst_color, dst_factor)
    //   maps to OpenGL: (srcRGB, dstRGB, srcAlpha, dstAlpha)
    GLenum srcRGB, dstRGB, srcAlpha, dstAlpha;
    if (mClipMaskMode) {
        // MASK path: always use NORMAL blending
        srcRGB = GL_ONE;
        dstRGB = GL_ONE_MINUS_SRC_ALPHA;
        srcAlpha = GL_ONE;
        dstAlpha = GL_ONE_MINUS_SRC_ALPHA;
    } else {
        switch (compositionType) {
            case COLOR_COMPOSITION_NORMAL:
                srcRGB = GL_ONE;
                dstRGB = GL_ONE_MINUS_SRC_ALPHA;
                srcAlpha = GL_ONE;
                dstAlpha = GL_ONE_MINUS_SRC_ALPHA;
                break;
            case COLOR_COMPOSITION_SCREEN:
                srcRGB = GL_ONE;
                dstRGB = GL_ONE;
                srcAlpha = GL_ZERO;
                dstAlpha = GL_ONE;
                break;
            case COLOR_COMPOSITION_MULTIPLY:
                srcRGB = GL_DST_COLOR;
                dstRGB = GL_ONE_MINUS_SRC_ALPHA;
                srcAlpha = GL_ZERO;
                dstAlpha = GL_ONE;
                break;
            default:
                LOGE("Unsupported composition type: %d", compositionType);
                srcRGB = GL_ONE;
                dstRGB = GL_ONE_MINUS_SRC_ALPHA;
                srcAlpha = GL_ONE;
                dstAlpha = GL_ONE_MINUS_SRC_ALPHA;
                break;
        }
    }

    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);

    auto& drawObject = mDrawObjects[drawbaleIndex];
    glBindVertexArray(drawObject.vao);
    glBindBuffer(GL_ARRAY_BUFFER, drawObject.vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertexFloatCount * sizeof(float), vertices);

    glDrawElements(GL_TRIANGLES, (GLsizei)indexCount, GL_UNSIGNED_SHORT, 0);
}

void GLRenderer::Resize(int w, int h) {
    (void)w;
    (void)h;
}

void GLRenderer::PreDraw(Moc* moc) {
    if (mClipManager) {
        SetupDraw(moc);
        mClipManager->SetupClip(moc);
    }
}

void GLRenderer::DrawDrawable(Moc* moc, int idx) {
    // clip 状态切换仅正常绘制路径需要；mask 渲染路径不消费（旧路径 mask 源自身
    // 带 clip 时会覆盖 matrixForMask，属无人触发角落，此处不再复现）
    if (!mClipMaskMode) {
        ClipContext* cc = mClipManager ? mClipManager->GetClipForDrawable(idx) : nullptr;
        if (cc) {
            SetClipChannel(cc->mLayoutChannelNo);
            SetClipMatrix(cc->mMatrixForDraw.data());
        }
        SetClipDrawMode(cc != nullptr);
    }
    SetCulling(moc->GetDrawableCulling(idx));

    int vertexFloatCount = moc->GetDrawableVertexCount(idx) * 2;
    DrawTexture(idx, 
        moc->GetDrawableTextureIndex(idx),
                moc->GetDrawableScreenColor(idx),
                moc->GetDrawableVertexIndices(idx),
                moc->GetDrawableVertexIndexCount(idx),
                moc->GetDrawableVertices(idx),
                vertexFloatCount,
                moc->GetDrawableVertexUvs(idx),
                vertexFloatCount,
                moc->GetDrawableOpacity(idx),
                moc->GetDrawableColorComposition(idx),
                moc->GetDrawableMultiplyColor(idx));
}

void GLRenderer::Draw(Moc* moc) {
    if (moc->GetDrawableCount() == 0)
        return;
    SetupDraw(moc);
    int seqCount = 0;
    const int* seq = moc->GetRenderSequence(seqCount);
    for (int i = 0; i < seqCount; i++)
        DrawDrawable(moc, seq[i]);
    EndDraw();
}

}   // namespace V2
}   // namespace Live2D