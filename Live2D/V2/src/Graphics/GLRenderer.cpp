#include "GLRenderer.hpp"
#include "../Common/Log.hpp"
#include "ClippingManagerOpenGL.hpp"
#include "Core/PartsData.hpp"
#include "Core/PartsDataContext.hpp"
#include "Draw/Mesh.hpp"
#include "Draw/MeshContext.hpp"
#include "Model/ModelContext.hpp"
#include <cstdio>
#include <cstring>
#ifdef __ANDROID__
#include <GLES3/gl3.h>
#endif


namespace live2d {

// GLSL shaders – exact match of Python v2 draw_param_opengl.py
// Version string (first line of each shader)
#define SHADER_VER "#version 120\n"

// -- Normal vertex (aK in Python) -- v_clipPos = gl_Position
static const char* sVertNormal =
    SHADER_VER "attribute vec2 a_position;"
               "attribute vec2 a_texCoord;"
               "varying vec2 v_texCoord;"
               "varying vec4 v_clipPos;"
               "uniform mat4 u_mvpMatrix;"
               "void main(){"
               "    gl_Position = u_mvpMatrix * vec4(a_position, 0.0, 1.0);"
               "    v_clipPos = gl_Position;"
               "    v_texCoord = a_texCoord;"
               "    v_texCoord.y = 1.0 - v_texCoord.y;"
               "}";

// -- Normal fragment (aM in Python) -- with u_maskFlag for mask rendering pass
static const char* sFragNormal = SHADER_VER
    "#ifdef GL_ES\n"
    "precision mediump float;\n"
    "#endif\n"
    "varying vec2 v_texCoord;"
    "varying vec4 v_clipPos;"
    "uniform sampler2D s_texture0;"
    "uniform vec4 u_channelFlag;"
    "uniform vec4 u_baseColor;"
    "uniform bool u_maskFlag;"
    "uniform vec4 u_screenColor;"
    "uniform vec4 u_multiplyColor;"
    "void main(){"
    "    vec4 smpColor;"
    "    if(u_maskFlag){"
    "        float isInside = "
    "            step(u_baseColor.x, v_clipPos.x/v_clipPos.w)"
    "          * step(u_baseColor.y, v_clipPos.y/v_clipPos.w)"
    "          * step(v_clipPos.x/v_clipPos.w, u_baseColor.z)"
    "          * step(v_clipPos.y/v_clipPos.w, u_baseColor.w);"
    "        smpColor = u_channelFlag * texture2D(s_texture0, v_texCoord).a * isInside;"
    "    }else{"
    "        smpColor = texture2D(s_texture0, v_texCoord);"
    "        smpColor.rgb = smpColor.rgb * smpColor.a;"
    "        smpColor.rgb = smpColor.rgb * u_multiplyColor.rgb;"
    "        smpColor.rgb = smpColor.rgb + u_screenColor.rgb - (smpColor.rgb * u_screenColor.rgb);"
    "        smpColor = smpColor * u_baseColor;"
    "    }"
    "    gl_FragColor = smpColor;"
    "}";

// -- Mask vertex (aL in Python) -- v_clipPos = u_clipMatrix * pos; u_mvpMatrix for position
static const char* sVertMask = SHADER_VER "attribute vec2 a_position;"
                                          "attribute vec2 a_texCoord;"
                                          "varying vec2 v_texCoord;"
                                          "varying vec4 v_clipPos;"
                                          "uniform mat4 u_mvpMatrix;"
                                          "uniform mat4 u_clipMatrix;"
                                          "void main(){"
                                          "    vec4 pos = vec4(a_position, 0.0, 1.0);"
                                          "    gl_Position = u_mvpMatrix * pos;"
                                          "    v_clipPos = u_clipMatrix * pos;"
                                          "    v_texCoord = a_texCoord;"
                                          "    v_texCoord.y = 1.0 - v_texCoord.y;"
                                          "}";

// -- Mask fragment (aJ in Python)
static const char* sFragMask = SHADER_VER
    "#ifdef GL_ES\n"
    "precision mediump float;\n"
    "#endif\n"
    "varying vec2 v_texCoord;"
    "varying vec4 v_clipPos;"
    "uniform sampler2D s_texture0;"
    "uniform sampler2D s_texture1;"
    "uniform vec4 u_channelFlag;"
    "uniform vec4 u_baseColor;"
    "uniform vec4 u_screenColor;"
    "uniform vec4 u_multiplyColor;"
    "void main(){"
    "    vec4 col_formask = texture2D(s_texture0, v_texCoord);"
    "    col_formask.rgb = col_formask.rgb * col_formask.a;"
    "    col_formask.rgb = col_formask.rgb * u_multiplyColor.rgb;"
    "    col_formask.rgb = col_formask.rgb + u_screenColor.rgb - (col_formask.rgb * "
    "u_screenColor.rgb);"
    "    col_formask = col_formask * u_baseColor;"
    "    vec4 clipMask = texture2D(s_texture1, v_clipPos.xy / v_clipPos.w) * u_channelFlag;"
    "    float maskVal = clipMask.r + clipMask.g + clipMask.b + clipMask.a;"
    "    col_formask = col_formask * maskVal;"
    "    gl_FragColor = col_formask;"
    "}";

GLRenderer::GLRenderer(ModelContext* modelContext, int textureCount)
    : mShaderNormal(0)
    , mShaderMask(0)
    , mFramebuffer(0)
    , mFramebufferTexture(0)
    , mPosVBO(0)
    , mUVVBO(0)
    , mEBO(0)
    , mCulling(false)
{
    mTextures.resize(textureCount, 0);
    init(modelContext);
}

void GLRenderer::init(ModelContext* modelContext)
{
    initShaders();

    mClipManager = std::make_unique<ClippingManagerOpenGL>(*this);
    std::vector<MeshContext*> rawDrawCtxs;
    rawDrawCtxs.reserve(modelContext->mDrawContextList.size());
    for (auto& ctx : modelContext->mDrawContextList) rawDrawCtxs.push_back(ctx.get());
    mClipManager->init(modelContext, modelContext->mDrawDataList, rawDrawCtxs);
}

GLRenderer::~GLRenderer()
{
    if (mShaderNormal)
        glDeleteProgram(mShaderNormal);
    if (mShaderMask)
        glDeleteProgram(mShaderMask);
    if (mFramebuffer)
        glDeleteFramebuffers(1, &mFramebuffer);
    if (mFramebufferTexture)
        glDeleteTextures(1, &mFramebufferTexture);
    if (mEBO)
        glDeleteBuffers(1, &mEBO);
    if (mPosVBO)
        glDeleteBuffers(1, &mPosVBO);
    if (mUVVBO)
        glDeleteBuffers(1, &mUVVBO);
    for (auto t : mTextures)
        if (t)
            glDeleteTextures(1, &t);
}

GLuint GLRenderer::compileShader(GLenum type, const char* src)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char buf[512];
        glGetShaderInfoLog(shader, 512, nullptr, buf);
        Error("Shader compile error (%s): %s\n",
                type == GL_VERTEX_SHADER ? "vertex" : "fragment",
                buf);
    }
    return shader;
}

void GLRenderer::initShaders()
{
    // Normal program: aK vertex + aM fragment
    GLuint vsNorm = compileShader(GL_VERTEX_SHADER, sVertNormal);
    GLuint fsNorm = compileShader(GL_FRAGMENT_SHADER, sFragNormal);
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
        Error("Normal program link error: %s\n", buf);
    }
    glDeleteShader(vsNorm);
    glDeleteShader(fsNorm);

    // Mask program: aL vertex + aJ fragment
    GLuint vsMask = compileShader(GL_VERTEX_SHADER, sVertMask);
    GLuint fsMask = compileShader(GL_FRAGMENT_SHADER, sFragMask);
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
        Error("Mask program link error: %s\n", buf);
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

GLuint GLRenderer::createVBO(const std::vector<float>& data, int loc, int size)
{
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(loc, size, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(loc);
    return vbo;
}

GLuint GLRenderer::createEBO(const std::vector<int16_t>& data)
{
    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER, data.size() * sizeof(int16_t), data.data(), GL_STATIC_DRAW);
    return ebo;
}

void GLRenderer::clearBuffer(float r, float g, float b, float a)
{
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void GLRenderer::setupDraw(ModelContext* modelContext)
{
    glGetIntegerv(GL_CURRENT_PROGRAM, reinterpret_cast<GLint*>(&mCurrentProgram));
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, reinterpret_cast<GLint*>(&mCurrentFBO));
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DEPTH_TEST);
    glFrontFace(GL_CW);
    glEnable(GL_BLEND);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void GLRenderer::endDraw()
{
    glBindFramebuffer(GL_FRAMEBUFFER, mCurrentFBO);
    // When changing model: v2cpp => v3,
    // v3 may get save the wrong program id to `lastProgramId`
    // and produce an silent gl error when restore `lastProgramId`.
    // This error will be checked and raised in v2.
    // Thus, re-bind program to `0` 
    glUseProgram(mCurrentProgram);
}

void GLRenderer::setMatrix(const float m[16])
{
    std::memcpy(mMatrix4x4.data(), m, 16 * sizeof(float));
}

void GLRenderer::setClipMatrix(const float m[16])
{
    std::memcpy(mClipMatrix.data(), m, 16 * sizeof(float));
}

int GLRenderer::createFramebuffer()
{
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

void GLRenderer::bindFramebuffer(int fb)
{
    if (fb == -1)
        fb = mFramebuffer;
    glBindFramebuffer(GL_FRAMEBUFFER, fb ? fb : mCurrentFBO);
}

void GLRenderer::setTexture(int no, GLuint texId)
{
    if (no >= (int)mTextures.size())
        mTextures.resize(no + 1, 0);
    mTextures[no] = texId;
}

GLuint GLRenderer::getTexture(int no) const
{
    if (no >= 0 && no < (int)mTextures.size())
        return mTextures[no];
    return 0;
}

void GLRenderer::drawTexture(int texNo, const std::array<float, 4>& screenColor,
                             const std::vector<int16_t>& indices,
                             const std::vector<float>& vertices, const std::vector<float>& uvs,
                             float opacity, int compositionType,
                             const std::array<float, 4>& multiplyColor)
{
    if (opacity < 0.01f && mClipMaskCtx == nullptr)
        return;

    float a_w = mBaseRed * opacity;
    float a2 = mBaseGreen * opacity;
    float a5 = mBaseBlue * opacity;
    float a7 = mBaseAlpha * opacity;

    static int sDrawCallNo = 0;
    sDrawCallNo++;
    int callNo = sDrawCallNo;
    const char* path = mClipMaskCtx ? "MASK" : mClipDrawCtx ? "CLIP" : "NORM";


    if (mClipMaskCtx) {
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
        glBindTexture(GL_TEXTURE_2D, getTexture(texNo));
        glUniform1i(mUniforms.normTexture0, 1);
    } else if (mClipDrawCtx) {
        // Path 2: Clipped DRAW — uses mask FBO
        glUseProgram(mShaderMask);
        glUniformMatrix4fv(mUniforms.maskMvp, 1, GL_FALSE, mMatrix4x4.data());
        glUniformMatrix4fv(
            mUniforms.maskClipMatrix, 1, GL_FALSE, mClipMatrix.data());
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
        glBindTexture(GL_TEXTURE_2D, getTexture(texNo));
        glUniform1i(mUniforms.maskTexture0, 1);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, mFramebufferTexture);
        glUniform1i(mUniforms.maskTexture1, 2);
        mClipDrawCtx = nullptr;
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
        glBindTexture(GL_TEXTURE_2D, getTexture(texNo));
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
    if (mClipMaskCtx) {
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
            Error("Unsupported composition type: %d", compositionType);
            srcRGB = GL_ONE;
            dstRGB = GL_ONE_MINUS_SRC_ALPHA;
            srcAlpha = GL_ONE;
            dstAlpha = GL_ONE_MINUS_SRC_ALPHA;
            break;
        }
    }

    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);

    // Debug: print blend function
    auto blendName = [](GLenum e) -> const char* {
        switch (e) {
        case GL_ONE:
            return "ONE";
        case GL_ZERO:
            return "ZERO";
        case GL_DST_COLOR:
            return "DST_COLOR";
        case GL_ONE_MINUS_SRC_ALPHA:
            return "ONE_MINUS_SRC_ALPHA";
        case GL_ONE_MINUS_SRC_COLOR:
            return "ONE_MINUS_SRC_COLOR";
        default:
            return "?";
        }
    };
    // Position VBO (location 0)
    if (!mPosVBO)
        glGenBuffers(1, &mPosVBO);
    glBindBuffer(GL_ARRAY_BUFFER, mPosVBO);
    glBufferData(
        GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    // UV VBO (location 1)
    if (!mUVVBO)
        glGenBuffers(1, &mUVVBO);
    glBindBuffer(GL_ARRAY_BUFFER, mUVVBO);
    glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(float), uvs.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);

    // EBO
    if (!mEBO)
        glGenBuffers(1, &mEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int16_t), indices.data(), GL_STATIC_DRAW);

    glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_SHORT, 0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
}
void GLRenderer::resize(int w, int h)
{
    (void)w;
    (void)h;
}

void GLRenderer::preDraw(ModelContext* context)
{
    if (mClipManager) {
        setupDraw(context);
        mClipManager->setupClip(context);
    }
}

void GLRenderer::draw(ModelContext* context)
{
    if (context->mOrderListFirstDrawIndex.empty())
        return;
    setupDraw(context);
    for (size_t i = 0; i < context->mOrderListFirstDrawIndex.size(); i++) {
        int idx = context->mOrderListFirstDrawIndex[i];
        if (idx == ModelContext::NOT_USED_ORDER)
            continue;
        while (true) {
            auto* dd = static_cast<Mesh*>(context->mDrawDataList[idx]);
            auto* ctx = context->mDrawContextList[idx].get();
            const char* pid =
                context->mPartsContextList[ctx->mPartsIndex]->mPartsData->getId()->str().c_str();
            if (ctx->mAvailable) {
                ctx->mPartsOpacity =
                    context->mPartsContextList[ctx->mPartsIndex]->getPartsOpacity();
                dd->draw(this, context, ctx);
            }
            int next = context->mNextListDrawIndex[idx];
            if (next <= idx || next == ModelContext::NO_NEXT)
                break;
            idx = next;
        }
    }
    endDraw();
}

}   // namespace live2d
