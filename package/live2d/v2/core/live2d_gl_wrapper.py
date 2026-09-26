import OpenGL.GL as gl
import numpy as np


class Live2DGLWrapper:
    """
    live2d 用到的所有 api
    """
    FRAMEBUFFER = gl.GL_FRAMEBUFFER
    RENDERBUFFER = gl.GL_RENDERBUFFER
    COLOR_BUFFER_BIT = gl.GL_COLOR_BUFFER_BIT
    RGBA4 = gl.GL_RGBA4
    COLOR_ATTACHMENT0 = gl.GL_COLOR_ATTACHMENT0
    RGBA = gl.GL_RGBA
    UNSIGNED_BYTE = gl.GL_UNSIGNED_BYTE
    TEXTURE_2D = gl.GL_TEXTURE_2D
    TEXTURE_MIN_FILTER = gl.GL_TEXTURE_MIN_FILTER
    TEXTURE_MAG_FILTER = gl.GL_TEXTURE_MAG_FILTER
    LINEAR = gl.GL_LINEAR
    LINEAR_MIPMAP_NEAREST = gl.GL_LINEAR_MIPMAP_NEAREST
    CLAMP_TO_EDGE = gl.GL_CLAMP_TO_EDGE
    TEXTURE_WRAP_S = gl.GL_TEXTURE_WRAP_S
    TEXTURE_WRAP_T = gl.GL_TEXTURE_WRAP_T
    VERTEX_SHADER = gl.GL_VERTEX_SHADER
    FRAGMENT_SHADER = gl.GL_FRAGMENT_SHADER
    COMPILE_STATUS = gl.GL_COMPILE_STATUS
    LINK_STATUS = gl.GL_LINK_STATUS
    SCISSOR_TEST = gl.GL_SCISSOR_TEST
    STENCIL_TEST = gl.GL_STENCIL_TEST
    DEPTH_TEST = gl.GL_DEPTH_TEST
    CW = gl.GL_CW
    CCW = gl.GL_CCW
    BLEND = gl.GL_BLEND
    ARRAY_BUFFER = gl.GL_ARRAY_BUFFER
    ELEMENT_ARRAY_BUFFER = gl.GL_ELEMENT_ARRAY_BUFFER
    DYNAMIC_DRAW = gl.GL_DYNAMIC_DRAW
    FLOAT = gl.GL_FLOAT
    TEXTURE1 = gl.GL_TEXTURE1
    TEXTURE2 = gl.GL_TEXTURE2
    CULL_FACE = gl.GL_CULL_FACE
    ONE = gl.GL_ONE
    SRC_ALPHA = gl.GL_SRC_ALPHA
    ONE_MINUS_SRC_ALPHA = gl.GL_ONE_MINUS_SRC_ALPHA
    DST_COLOR = gl.GL_DST_COLOR
    ZERO = gl.GL_ZERO
    FUNC_ADD = gl.GL_FUNC_ADD
    TRIANGLES = gl.GL_TRIANGLES
    UNSIGNED_SHORT = gl.GL_UNSIGNED_SHORT
    FRAMEBUFFER_BINDING = gl.GL_FRAMEBUFFER_BINDING
    VIEWPORT = gl.GL_VIEWPORT
    DEPTH_BUFFER_BIT = gl.GL_DEPTH_BUFFER_BIT
    ONE_MINUS_SRC_COLOR = gl.GL_ONE_MINUS_SRC_COLOR
    BLEND_SRC_RGB = gl.GL_BLEND_SRC_RGB
    BLEND_SRC_ALPHA = gl.GL_BLEND_SRC_ALPHA
    BLEND_DST_RGB = gl.GL_BLEND_DST_RGB
    BLEND_DST_ALPHA = gl.GL_BLEND_DST_ALPHA

    def __init__(self):
        self.width = 0
        self.height = 0

    def resize(self, w, h):
        self.width = w
        self.height = h

    @staticmethod
    def getAttribLocation(program, name):
        return gl.glGetAttribLocation(program, name)

    @staticmethod
    def getUniformLocation(program, name):
        return gl.glGetUniformLocation(program, name)

    @staticmethod
    def createFramebuffer():
        return gl.glGenFramebuffers(1)

    @staticmethod
    def bindFramebuffer(t, fbo):
        gl.glBindFramebuffer(t, fbo)

    @staticmethod
    def createRenderbuffer():
        return gl.glGenRenderbuffers(1)

    @staticmethod
    def bindRenderbuffer(t, rbo):
        gl.glBindRenderbuffer(t, rbo)

    @staticmethod
    def renderbufferStorage(t, fat, width, height):
        gl.glRenderbufferStorage(t, fat, width, height)

    @staticmethod
    def framebufferRenderbuffer(t, attachment, rbt, rb):
        gl.glFramebufferRenderbuffer(t, attachment, rbt, rb)

    @staticmethod
    def createTexture():
        return gl.glGenTextures(1)

    @staticmethod
    def bindTexture(t, tid):
        gl.glBindTexture(t, tid)

    @staticmethod
    def texImage2D(*args):
        gl.glTexImage2D(*args)

    @staticmethod
    def texParameteri(*args):
        gl.glTexParameteri(*args)

    @staticmethod
    def generateMipmap(t):
        gl.glGenerateMipmap(t)

    @staticmethod
    def framebufferTexture2D(*args):
        gl.glFramebufferTexture2D(*args)

    @staticmethod
    def createProgram():
        return gl.glCreateProgram()

    @staticmethod
    def compileShader(*args):
        gl.glCompileShader(*args)

    @staticmethod
    def createShader(*args):
        return gl.glCreateShader(*args)

    @staticmethod
    def shaderSource(*args):
        gl.glShaderSource(*args)

    @staticmethod
    def getShaderParameter(*args):
        return gl.glGetShaderiv(*args)

    @staticmethod
    def getShaderInfoLog(*args):
        return gl.glGetShaderInfoLog(*args)

    @staticmethod
    def attachShader(*args):
        return gl.glAttachShader(*args)

    @staticmethod
    def linkProgram(*args):
        gl.glLinkProgram(*args)

    @staticmethod
    def getProgramParameter(*args):
        return gl.glGetProgramiv(*args)

    @staticmethod
    def getProgramInfoLog(*args):
        return gl.glGetProgramInfoLog(*args)

    @staticmethod
    def disable(t):
        gl.glDisable(t)

    @staticmethod
    def bindBuffer(*args):
        gl.glBindBuffer(*args)

    @staticmethod
    def enable(t):
        gl.glEnable(t)

    @staticmethod
    def colorMask(*args):
        gl.glColorMask(*args)

    @staticmethod
    def frontFace(t):
        gl.glFrontFace(t)

    @staticmethod
    def useProgram(p):
        gl.glUseProgram(p)

    @staticmethod
    def createBuffer():
        return gl.glGenBuffers(1)

    @staticmethod
    def bufferData(t, data, usage):
        if t == Live2DGLWrapper.ARRAY_BUFFER:
            buf = np.array(data, dtype=np.float32)
        elif t == Live2DGLWrapper.ELEMENT_ARRAY_BUFFER:
            buf = np.array(data, dtype=np.uint16)
        else:
            raise Exception()

        gl.glBufferData(t, buf.nbytes, buf, usage)

    @staticmethod
    def enableVertexAttribArray(vao):
        gl.glEnableVertexAttribArray(vao)

    @staticmethod
    def vertexAttribPointer(*args):
        gl.glVertexAttribPointer(*args)

    @staticmethod
    def activeTexture(t):
        gl.glActiveTexture(t)

    @staticmethod
    def uniform1i(*args):
        gl.glUniform1i(*args)

    @staticmethod
    def uniformMatrix4fv(loc, transpose, value):
        buf = np.array(value, dtype=np.float32)
        gl.glUniformMatrix4fv(loc, 1, transpose, buf)

    @staticmethod
    def uniform4f(loc, *args):
        gl.glUniform4f(loc, *args)

    @staticmethod
    def blendEquationSeparate(a, b):
        gl.glBlendEquationSeparate(a, b)

    @staticmethod
    def blendFuncSeparate(a, b, c, d):
        gl.glBlendFuncSeparate(a, b, c, d)

    @staticmethod
    def drawElements(t, size, dt, data):
        gl.glDrawElements(t, size, dt, data)

    @staticmethod
    def getParameter(t):
        return gl.glGetIntegerv(t)

    @staticmethod
    def viewport(a, b, c, d):
        gl.glViewport(a, b, c, d)

    @staticmethod
    def clearColor(a, b, c, d):
        gl.glClearColor(a, b, c, d)

    @staticmethod
    def clear(t):
        gl.glClear(t)

    @staticmethod
    def deleteFramebuffer(t):
        gl.glDeleteFramebuffers(1, t)

    @staticmethod
    def deleteShader(s):
        gl.glDeleteShader(s)

    @staticmethod
    def deleteTexture(t):
        gl.glDeleteTextures(1, t)

    @staticmethod
    def deleteBuffer(b):
        gl.glDeleteBuffers(1, b)

    @staticmethod
    def deleteProgram(p):
        gl.glDeleteProgram(p)

    @staticmethod
    def deleteRenderbuffer(r):
        gl.glDeleteRenderbuffers(1, r)
