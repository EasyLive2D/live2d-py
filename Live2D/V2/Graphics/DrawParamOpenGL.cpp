#include "DrawParamOpenGL.hpp"
#include <cstdio>
#include <cstring>
#include <stdexcept>

namespace live2d {

// GLSL shaders – exact match of Python v2 draw_param_opengl.py
// Version string (first line of each shader)
#define SHADER_VER "#version 120\n"

// -- Normal vertex (aK in Python) -- v_clipPos = gl_Position
static const char* sVertNormal = SHADER_VER
"attribute vec2 a_position;"
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
"precision mediump float;"
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
static const char* sVertMask = SHADER_VER
"attribute vec2 a_position;"
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
"precision mediump float;"
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
"    col_formask.rgb = col_formask.rgb + u_screenColor.rgb - (col_formask.rgb * u_screenColor.rgb);"
"    col_formask = col_formask * u_baseColor;"
"    vec4 clipMask = texture2D(s_texture1, v_clipPos.xy / v_clipPos.w) * u_channelFlag;"
"    float maskVal = clipMask.r + clipMask.g + clipMask.b + clipMask.a;"
"    col_formask = col_formask * maskVal;"
"    gl_FragColor = col_formask;"
"}";

DrawParamOpenGL::DrawParamOpenGL() {
    mTextures.resize(32, 0);
}

void DrawParamOpenGL::lazyInit() {
    if (mInited) return;
    mInited = true;
    initShader();
}

DrawParamOpenGL::~DrawParamOpenGL() {
    if (mShaderNormal) glDeleteProgram(mShaderNormal);
    if (mShaderMask) glDeleteProgram(mShaderMask);
    if (mFramebuffer) glDeleteFramebuffers(1, &mFramebuffer);
    if (mFramebufferTexture) glDeleteTextures(1, &mFramebufferTexture);
    if (mEBO) glDeleteBuffers(1, &mEBO);
    if (mPosVBO) glDeleteBuffers(1, &mPosVBO);
    if (mUVVBO) glDeleteBuffers(1, &mUVVBO);
    for (auto t : mTextures) if (t) glDeleteTextures(1, &t);
}

GLuint DrawParamOpenGL::compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char buf[512];
        glGetShaderInfoLog(shader, 512, nullptr, buf);
    }
    return shader;
}

void DrawParamOpenGL::initShader() {
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
        fprintf(stderr, "Normal program link error: %s\n", buf);
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
        fprintf(stderr, "Mask program link error: %s\n", buf);
    }
    glDeleteShader(vsMask);
    glDeleteShader(fsMask);
}

GLuint DrawParamOpenGL::createVBO(const std::vector<float>& data, int loc, int size) {
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(loc, size, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(loc);
    return vbo;
}

GLuint DrawParamOpenGL::createEBO(const std::vector<int16_t>& data) {
    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, data.size() * sizeof(int16_t), data.data(), GL_STATIC_DRAW);
    return ebo;
}

void DrawParamOpenGL::clearBuffer(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void DrawParamOpenGL::setupDraw() {
    lazyInit();
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&mCurrentFBO);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DEPTH_TEST);
    glFrontFace(GL_CW);
    glEnable(GL_BLEND);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void DrawParamOpenGL::endDraw() {
    glBindFramebuffer(GL_FRAMEBUFFER, mCurrentFBO);
}

void DrawParamOpenGL::setMatrix(const float m[16]) {
    std::memcpy(mMatrix4x4.data(), m, 16 * sizeof(float));
}

void DrawParamOpenGL::setClipMatrix(const float m[16]) {
    std::memcpy(mClipMatrix.data(), m, 16 * sizeof(float));
}

int DrawParamOpenGL::createFramebuffer() {
    if (mFramebuffer == 0) glGenFramebuffers(1, &mFramebuffer);
    if (mFramebufferTexture == 0) glGenTextures(1, &mFramebufferTexture);

    glBindTexture(GL_TEXTURE_2D, mFramebufferTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mFramebufferTexture, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, mCurrentFBO);

    return (int)mFramebuffer;
}

void DrawParamOpenGL::bindFramebuffer(int fb) {
    glBindFramebuffer(GL_FRAMEBUFFER, fb ? fb : mCurrentFBO);
}

void DrawParamOpenGL::setTexture(int no, GLuint texId) {
    if (no >= (int)mTextures.size()) mTextures.resize(no + 1, 0);
    mTextures[no] = texId;
}

GLuint DrawParamOpenGL::getTexture(int no) const {
    if (no >= 0 && no < (int)mTextures.size()) return mTextures[no];
    return 0;
}

void DrawParamOpenGL::drawTexture(int texNo, const std::array<float, 4>& screenColor,
                                  const std::vector<int16_t>& indices,
                                  const std::vector<float>& vertices,
                                  const std::vector<float>& uvs,
                                  float opacity, int compositionType,
                                  const std::array<float, 4>& multiplyColor) {
    if (opacity < 0.01f && mClipMaskCtx == nullptr) return;

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
        GLuint p = mShaderNormal;
        glUniformMatrix4fv(glGetUniformLocation(p, "u_mvpMatrix"), 1, GL_FALSE, mClipMatrix.data());
        glUniform1i(glGetUniformLocation(p, "u_maskFlag"), 1);
        // u_baseColor = clip rect (full [-1,1] for full-FBO mask render)
        glUniform4f(glGetUniformLocation(p, "u_baseColor"), -1.0f, -1.0f, 1.0f, 1.0f);
        float chR=(mClipChannel==0)?1.0f:0,chG=(mClipChannel==1)?1.0f:0,chB=(mClipChannel==2)?1.0f:0,chA=(mClipChannel==3)?1.0f:0;
        // Channel color: R=ch0, G=ch1, B=ch2, A=ch3
        float cR=(mClipChannel==0)?1.0f:0, cG=(mClipChannel==1)?1.0f:0,
              cB=(mClipChannel==2)?1.0f:0, cA=(mClipChannel==3)?1.0f:0;
        glUniform4f(glGetUniformLocation(p, "u_channelFlag"), cR, cG, cB, cA);
        glUniform4f(glGetUniformLocation(p, "u_screenColor"), 0,0,0,0);
        glUniform4f(glGetUniformLocation(p, "u_multiplyColor"), 1,1,1,0);
        glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, getTexture(texNo));
        glUniform1i(glGetUniformLocation(p, "s_texture0"), 1);
    } else if (mClipDrawCtx) {
        // Path 2: Clipped DRAW — uses mask FBO
        glUseProgram(mShaderMask);
        GLuint p = mShaderMask;
        glUniformMatrix4fv(glGetUniformLocation(p, "u_mvpMatrix"), 1, GL_FALSE, mMatrix4x4.data());
        glUniformMatrix4fv(glGetUniformLocation(p, "u_clipMatrix"), 1, GL_FALSE, mClipMatrix.data());
        glUniform4f(glGetUniformLocation(p, "u_baseColor"), a_w, a2, a5, a7);
        glUniform4f(glGetUniformLocation(p, "u_screenColor"), screenColor[0], screenColor[1], screenColor[2], screenColor[3]);
        glUniform4f(glGetUniformLocation(p, "u_multiplyColor"), multiplyColor[0], multiplyColor[1], multiplyColor[2], multiplyColor[3]);
        float chR=(mClipChannel==0)?1.0f:0,chG=(mClipChannel==1)?1.0f:0,chB=(mClipChannel==2)?1.0f:0,chA=(mClipChannel==3)?1.0f:0;
        glUniform4f(glGetUniformLocation(p, "u_channelFlag"), chR, chG, chB, chA);
        glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, getTexture(texNo));
        glUniform1i(glGetUniformLocation(p, "s_texture0"), 1);
        glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, mFramebufferTexture);
        glUniform1i(glGetUniformLocation(p, "s_texture1"), 2);
        mClipDrawCtx = nullptr;
    } else {
        // Path 3: Normal draw
        glUseProgram(mShaderNormal);
        GLuint p = mShaderNormal;
        glUniformMatrix4fv(glGetUniformLocation(p, "u_mvpMatrix"), 1, GL_FALSE, mMatrix4x4.data());
        glUniform1i(glGetUniformLocation(p, "u_maskFlag"), 0);
        glUniform4f(glGetUniformLocation(p, "u_baseColor"), a_w, a2, a5, a7);
        glUniform4f(glGetUniformLocation(p, "u_screenColor"), screenColor[0], screenColor[1], screenColor[2], screenColor[3]);
        glUniform4f(glGetUniformLocation(p, "u_multiplyColor"), multiplyColor[0], multiplyColor[1], multiplyColor[2], multiplyColor[3]);
        glUniform4f(glGetUniformLocation(p, "u_channelFlag"), 0,0,0,0);
        glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, getTexture(texNo));
        glUniform1i(glGetUniformLocation(p, "s_texture0"), 1);
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
                throw std::runtime_error("Unsupported composition type: " + std::to_string(compositionType));
        }
    }

    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);

    // Debug: print blend function
    auto blendName = [](GLenum e) -> const char* {
        switch(e) {
            case GL_ONE: return "ONE";
            case GL_ZERO: return "ZERO";
            case GL_DST_COLOR: return "DST_COLOR";
            case GL_ONE_MINUS_SRC_ALPHA: return "ONE_MINUS_SRC_ALPHA";
            case GL_ONE_MINUS_SRC_COLOR: return "ONE_MINUS_SRC_COLOR";
            default: return "?";
        }
    };
    // Position VBO (location 0)
    if (!mPosVBO) glGenBuffers(1, &mPosVBO);
    glBindBuffer(GL_ARRAY_BUFFER, mPosVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    // UV VBO (location 1)
    if (!mUVVBO) glGenBuffers(1, &mUVVBO);
    glBindBuffer(GL_ARRAY_BUFFER, mUVVBO);
    glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(float), uvs.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);

    // EBO
    if (!mEBO) glGenBuffers(1, &mEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int16_t), indices.data(), GL_STATIC_DRAW);

    // Debug: verify actual GL blend state
    static int sBlendCheck = 0;
    if (getenv("V2CPP_DUMP") && sBlendCheck < 80) {
        GLint actualSrcRGB, actualDstRGB, actualSrcA, actualDstA;
        GLint actualEqRGB, actualEqA;
        glGetIntegerv(GL_BLEND_SRC_RGB, &actualSrcRGB);
        glGetIntegerv(GL_BLEND_DST_RGB, &actualDstRGB);
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &actualSrcA);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &actualDstA);
        glGetIntegerv(GL_BLEND_EQUATION_RGB, &actualEqRGB);
        glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &actualEqA);
        GLint srgb; glGetIntegerv(GL_FRAMEBUFFER_SRGB, &srgb);
        fprintf(stderr, "  [BLEND_CHECK#%d] set=(%d,%d,%d,%d) actual=(%d,%d,%d,%d) eq=(%d,%d) sRGB=%d\n",
            sBlendCheck, srcRGB, dstRGB, srcAlpha, dstAlpha,
            actualSrcRGB, actualDstRGB, actualSrcA, actualDstA,
            actualEqRGB, actualEqA, srgb);
        sBlendCheck++;
    }
    glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_SHORT, 0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
}

} // namespace live2d
