from ..draw import Mesh
from ..live2d import Live2D
from ..live2d_gl_wrapper import Live2DGLWrapper
from .draw_param import DrawParam


class FrameBufferObject:

    def __init__(self, fbo, rbo, tex):
        self.framebuffer = fbo
        self.renderbuffer = rbo
        self.texture = tex


class DrawParamOpenGL(DrawParam):

    def __init__(self, version: str, disable_precision: bool):
        super().__init__()
        self.framebufferObject: FrameBufferObject | None = None
        self.shaderProgramOff = None
        self.textures = []
        self.transform = None
        self.gl = Live2DGLWrapper()
        self.firstDraw = True
        self.anisotropyExt = None
        self.maxAnisotropy = 0
        self.uvbo = None
        self.vbo = None
        self.ebo = None
        self.vertShader = None
        self.fragShader = None
        self.vertShaderOff = None
        self.fragShaderOff = None
        self.lastBlending = [0, 0, 0, 0]

        precision = ""

        if not disable_precision:
            precision = "precision mediump float;"

        self.aK = (version +
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
              "}")
        self.aM = (version +
              precision +
              "varying vec2       v_texCoord;"
              "varying vec4       v_clipPos;"
              "uniform sampler2D  s_texture0;"
              "uniform vec4       u_channelFlag;"
              "uniform vec4       u_baseColor;"
              "uniform bool       u_maskFlag;"
              "uniform vec4       u_screenColor;"
              "uniform vec4       u_multiplyColor;"
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
              "        smpColor = texture2D(s_texture0 , v_texCoord);"
              "        smpColor.rgb = smpColor.rgb * smpColor.a;"
              "        smpColor.rgb = smpColor.rgb * u_multiplyColor.rgb;"
              "        smpColor.rgb = smpColor.rgb + u_screenColor.rgb - (smpColor.rgb * u_screenColor.rgb);"
              "        smpColor = smpColor * u_baseColor;"
              "    }"
              "    gl_FragColor = smpColor;}")
        self.aL = (version +
              "attribute vec2     a_position;"
              "attribute vec2     a_texCoord;"
              "varying vec2       v_texCoord;"
              "varying vec4       v_clipPos;"
              "uniform mat4       u_mvpMatrix;"
              "uniform mat4       u_clipMatrix;"
              "void main(){"
              "    vec4 pos = vec4(a_position, 0, 1.0);"
              "    gl_Position = u_mvpMatrix * pos;"
              "    v_clipPos = u_clipMatrix * pos;"
              "    v_texCoord = a_texCoord;"
              "    v_texCoord.y = 1.0 - v_texCoord.y;"
              "}"
              )
        self.aJ = (version +
              precision +
              "varying   vec2   v_texCoord;"
              "varying   vec4   v_clipPos;"
              "uniform sampler2D  s_texture0;"
              "uniform sampler2D  s_texture1;"
              "uniform vec4       u_channelFlag;"
              "uniform vec4       u_baseColor;"
              "uniform vec4       u_screenColor;"
              "uniform vec4       u_multiplyColor;"
              "void main(){"
              "    vec4 col_formask = texture2D(s_texture0, v_texCoord);"
              "    col_formask.rgb = col_formask.rgb * col_formask.a;"
              "    col_formask.rgb = col_formask.rgb * u_multiplyColor.rgb;"
              "    col_formask.rgb = col_formask.rgb + u_screenColor.rgb - (col_formask.rgb * u_screenColor.rgb);"
              "    col_formask = col_formask * u_baseColor;"
              "    vec4 clipMask = texture2D(s_texture1, v_clipPos.xy / v_clipPos.w) * u_channelFlag;"
              "    float maskVal = clipMask.r + clipMask.g + clipMask.b + clipMask.a;"
              "    col_formask = col_formask * maskVal;"
              "    gl_FragColor = col_formask;}")

    def getGL(self):
        return self.gl

    def setGL(self, aH):
        self.gl = aH

    def resize(self, ww, wh):
        self.gl.resize(ww, wh)

    def setTransform(self, aH):
        self.transform = aH

    def setupDraw(self):
        a_h = self.gl
        if self.firstDraw:
            self.initShader()
            self.firstDraw = False
            # self.anisotropyExt = aH.MAX_TEXTURE_MAX_ANISOTROPY
            # if self.anisotropyExt:
            #     self.maxAnisotropy = a_h.getParameter(self.anisotropyExt)

        a_h.disable(a_h.SCISSOR_TEST)
        a_h.disable(a_h.STENCIL_TEST)
        a_h.disable(a_h.DEPTH_TEST)
        a_h.frontFace(a_h.CW)
        a_h.enable(a_h.BLEND)
        a_h.colorMask(1, 1, 1, 1)
        a_h.bindBuffer(a_h.ARRAY_BUFFER, 0)
        a_h.bindBuffer(a_h.ELEMENT_ARRAY_BUFFER, 0)
        self.lastBlending[0] = a_h.getParameter(a_h.BLEND_SRC_RGB)
        self.lastBlending[1] = a_h.getParameter(a_h.BLEND_SRC_ALPHA)
        self.lastBlending[2] = a_h.getParameter(a_h.BLEND_DST_RGB)
        self.lastBlending[3] = a_h.getParameter(a_h.BLEND_DST_ALPHA)

    def endDraw(self):
        self.gl.blendFuncSeparate(self.lastBlending[0], self.lastBlending[1], self.lastBlending[2], self.lastBlending[3])

    def drawTexture(self, texNo, screenColor, indexArray, vertexArray, uvArray, opacity, comp, multiplyColor):
        if opacity < 0.01 and self.clipBufPre_clipContextMask is None:
            return

        gl = self.gl

        a_w = self.baseRed * opacity
        a2 = self.baseGreen * opacity
        a5 = self.baseBlue * opacity
        a7 = self.baseAlpha * opacity
        # print("baseColor", a_w, a2, a5, a7)
        if self.clipBufPre_clipContextMask is not None:
            # 根据蒙版进行裁剪
            gl.frontFace(gl.CCW)
            gl.useProgram(self.shaderProgram)
            self.vbo = bindOrCreateVBO(gl, self.vbo, vertexArray)
            self.ebo = bindOrCreateEBO(gl, self.ebo, indexArray)
            gl.vertexAttribPointer(self.a_position_Loc, 2, gl.FLOAT, False, 0, None)
            gl.enableVertexAttribArray(self.a_position_Loc)
            self.uvbo = bindOrCreateVBO(gl, self.uvbo, uvArray)
            gl.activeTexture(gl.TEXTURE1)
            gl.bindTexture(gl.TEXTURE_2D, self.textures[texNo])
            gl.uniform1i(self.s_texture0_Loc, 1)
            gl.vertexAttribPointer(self.a_texCoord_Loc, 2, gl.FLOAT, False, 0, None)
            gl.enableVertexAttribArray(self.a_texCoord_Loc)
            gl.uniformMatrix4fv(self.u_matrix_Loc, False, self.getClipBufPre_clipContextMask().matrixForMask)
            aY = self.getClipBufPre_clipContextMask().layoutChannelNo
            a4 = self.getChannelFlagAsColor(aY)
            gl.uniform4f(self.u_channelFlag, a4.r, a4.g, a4.b, a4.a)
            aI = self.getClipBufPre_clipContextMask().layoutBounds
            gl.uniform4f(self.u_baseColor_Loc, aI.x * 2.0 - 1.0, aI.y * 2.0 - 1.0, aI.getRight() * 2.0 - 1.0,
                         aI.getBottom() * 2.0 - 1.0)
            gl.uniform1i(self.u_maskFlag_Loc, True)
            gl.uniform4f(self.u_screenColor_Loc, *screenColor)
            gl.uniform4f(self.u_multiplyColor_Loc, *multiplyColor)
        elif self.clipBufPre_clipContextDraw is not None:
            # 绘制蒙版
            gl.useProgram(self.shaderProgramOff)
            self.vbo = bindOrCreateVBO(gl, self.vbo, vertexArray)
            self.ebo = bindOrCreateEBO(gl, self.ebo, indexArray)
            gl.enableVertexAttribArray(self.a_position_Loc_Off)
            gl.vertexAttribPointer(self.a_position_Loc_Off, 2, gl.FLOAT, False, 0, None)
            self.uvbo = bindOrCreateVBO(gl, self.uvbo, uvArray)
            gl.activeTexture(gl.TEXTURE1)
            gl.bindTexture(gl.TEXTURE_2D, self.textures[texNo])
            gl.uniform1i(self.s_texture0_Loc_Off, 1)
            gl.enableVertexAttribArray(self.a_texCoord_Loc_Off)
            gl.vertexAttribPointer(self.a_texCoord_Loc_Off, 2, gl.FLOAT, False, 0, None)
            gl.uniformMatrix4fv(self.u_clipMatrix_Loc_Off, False,
                                self.getClipBufPre_clipContextDraw().matrixForDraw)
            gl.uniformMatrix4fv(self.u_matrix_Loc_Off, False, self.matrix4x4)
            gl.activeTexture(gl.TEXTURE2)
            gl.bindTexture(gl.TEXTURE_2D, self.framebufferObject.texture)
            gl.uniform1i(self.s_texture1_Loc_Off, 2)
            aY = self.getClipBufPre_clipContextDraw().layoutChannelNo
            a4 = self.getChannelFlagAsColor(aY)
            gl.uniform4f(self.u_channelFlag_Loc_Off, a4.r, a4.g, a4.b, a4.a)
            gl.uniform4f(self.u_baseColor_Loc_Off, a_w, a2, a5, a7)
            gl.uniform4f(self.u_screenColor_Loc_Off, *screenColor)
            gl.uniform4f(self.u_multiplyColor_Loc_Off, *multiplyColor)
        else:
            gl.useProgram(self.shaderProgram)
            self.vbo = bindOrCreateVBO(gl, self.vbo, vertexArray)
            self.ebo = bindOrCreateEBO(gl, self.ebo, indexArray)
            gl.enableVertexAttribArray(self.a_position_Loc)
            gl.vertexAttribPointer(self.a_position_Loc, 2, gl.FLOAT, False, 0, None)
            self.uvbo = bindOrCreateVBO(gl, self.uvbo, uvArray)
            gl.activeTexture(gl.TEXTURE1)
            gl.bindTexture(gl.TEXTURE_2D, self.textures[texNo])
            gl.uniform1i(self.s_texture0_Loc, 1)
            gl.enableVertexAttribArray(self.a_texCoord_Loc)
            gl.vertexAttribPointer(self.a_texCoord_Loc, 2, gl.FLOAT, False, 0, None)
            gl.uniformMatrix4fv(self.u_matrix_Loc, False, self.matrix4x4)
            gl.uniform4f(self.u_baseColor_Loc, a_w, a2, a5, a7)
            gl.uniform1i(self.u_maskFlag_Loc, False)
            gl.uniform4f(self.u_screenColor_Loc, *screenColor)
            gl.uniform4f(self.u_multiplyColor_Loc, *multiplyColor)
        if self.culling:
            self.gl.enable(gl.CULL_FACE)
        else:
            self.gl.disable(gl.CULL_FACE)

        self.gl.enable(gl.BLEND)
        src_color = None
        src_factor = None
        dst_color = None
        dst_factor = None

        if self.clipBufPre_clipContextMask is not None:
            src_color = gl.ONE
            src_factor = gl.ONE_MINUS_SRC_ALPHA
            dst_color = gl.ONE
            dst_factor = gl.ONE_MINUS_SRC_ALPHA
        else:
            if comp == Mesh.COLOR_COMPOSITION_NORMAL:
                src_color = gl.ONE
                src_factor = gl.ONE_MINUS_SRC_ALPHA
                dst_color = gl.ONE
                dst_factor = gl.ONE_MINUS_SRC_ALPHA
            elif comp == Mesh.COLOR_COMPOSITION_SCREEN:
                src_color = gl.ONE
                src_factor = gl.ONE
                dst_color = gl.ZERO
                dst_factor = gl.ONE
            elif comp == Mesh.COLOR_COMPOSITION_MULTIPLY:
                src_color = gl.DST_COLOR
                src_factor = gl.ONE_MINUS_SRC_ALPHA
                dst_color = gl.ZERO
                dst_factor = gl.ONE
            else:
                raise RuntimeError("unknown comp")

        gl.blendEquationSeparate(gl.FUNC_ADD, gl.FUNC_ADD)
        gl.blendFuncSeparate(src_color, src_factor, dst_color, dst_factor)
        # if self.anisotropyExt:
        #     g0.texParameteri(g0.TEXTURE_2D, self.anisotropyExt.TEXTURE_MAX_ANISOTROPY_EXT, self.maxAnisotropy)

        count = len(indexArray)
        gl.drawElements(gl.TRIANGLES, count, gl.UNSIGNED_SHORT, None)
        gl.bindTexture(gl.TEXTURE_2D, 0)

    def setTexture(self, aH, aI):
        size = len(self.textures)
        if aH >= size:
            for i in range(size, aH + 1):
                self.textures.append(None)
        self.textures[aH] = aI

    def initShader(self):
        aH = self.gl
        self.loadShaders2()
        self.a_position_Loc = aH.getAttribLocation(self.shaderProgram, "a_position")
        self.a_texCoord_Loc = aH.getAttribLocation(self.shaderProgram, "a_texCoord")
        self.u_matrix_Loc = aH.getUniformLocation(self.shaderProgram, "u_mvpMatrix")
        self.s_texture0_Loc = aH.getUniformLocation(self.shaderProgram, "s_texture0")
        self.u_channelFlag = aH.getUniformLocation(self.shaderProgram, "u_channelFlag")
        self.u_baseColor_Loc = aH.getUniformLocation(self.shaderProgram, "u_baseColor")
        self.u_maskFlag_Loc = aH.getUniformLocation(self.shaderProgram, "u_maskFlag")
        self.a_position_Loc_Off = aH.getAttribLocation(self.shaderProgramOff, "a_position")
        self.a_texCoord_Loc_Off = aH.getAttribLocation(self.shaderProgramOff, "a_texCoord")
        self.u_matrix_Loc_Off = aH.getUniformLocation(self.shaderProgramOff, "u_mvpMatrix")
        self.u_clipMatrix_Loc_Off = aH.getUniformLocation(self.shaderProgramOff, "u_clipMatrix")
        self.s_texture0_Loc_Off = aH.getUniformLocation(self.shaderProgramOff, "s_texture0")
        self.s_texture1_Loc_Off = aH.getUniformLocation(self.shaderProgramOff, "s_texture1")
        self.u_channelFlag_Loc_Off = aH.getUniformLocation(self.shaderProgramOff, "u_channelFlag")
        self.u_baseColor_Loc_Off = aH.getUniformLocation(self.shaderProgramOff, "u_baseColor")
        self.u_screenColor_Loc = aH.getUniformLocation(self.shaderProgram, "u_screenColor")
        self.u_screenColor_Loc_Off = aH.getUniformLocation(self.shaderProgramOff, "u_screenColor")
        self.u_multiplyColor_Loc = aH.getUniformLocation(self.shaderProgram, "u_multiplyColor")
        self.u_multiplyColor_Loc_Off = aH.getUniformLocation(self.shaderProgramOff, "u_multiplyColor")

    def disposeShader(self):
        aH = self.gl
        if self.shaderProgram:
            aH.deleteProgram(self.shaderProgram)
            self.shaderProgram = None

        if self.shaderProgramOff:
            aH.deleteProgram(self.shaderProgramOff)
            self.shaderProgramOff = None

    def compileShader(self, aJ, aN):
        aM = self.gl
        aL = aN
        aK = aM.createShader(aJ)
        if aK is None:
            print("_L0 to create shader")
            return None

        aM.shaderSource(aK, aL)
        aM.compileShader(aK)
        aH = aM.getShaderParameter(aK, aM.COMPILE_STATUS)
        if not aH:
            aI = aM.getShaderInfoLog(aK)
            print(f"_L0 to compile shader : {aI}")
            aM.deleteShader(aK)
            return None

        return aK

    def loadShaders2(self):
        aN = self.gl
        self.shaderProgram = aN.createProgram()
        if not self.shaderProgram:
            return False

        self.shaderProgramOff = aN.createProgram()
        if not self.shaderProgramOff:
            return False

        self.vertShader = self.compileShader(aN.VERTEX_SHADER, self.aK)
        if not self.vertShader:
            print("Vertex shader compile li_!")
            return False

        self.vertShaderOff = self.compileShader(aN.VERTEX_SHADER, self.aL)
        if not self.vertShaderOff:
            print("OffVertex shader compile li_!")
            return False

        self.fragShader = self.compileShader(aN.FRAGMENT_SHADER, self.aM)
        if not self.fragShader:
            print("Fragment shader compile li_!")
            return False

        self.fragShaderOff = self.compileShader(aN.FRAGMENT_SHADER, self.aJ)
        if not self.fragShaderOff:
            print("OffFragment shader compile li_!")
            return False

        aN.attachShader(self.shaderProgram, self.vertShader)
        aN.attachShader(self.shaderProgram, self.fragShader)
        aN.attachShader(self.shaderProgramOff, self.vertShaderOff)
        aN.attachShader(self.shaderProgramOff, self.fragShaderOff)
        aN.linkProgram(self.shaderProgram)
        aN.linkProgram(self.shaderProgramOff)
        aH = aN.getProgramParameter(self.shaderProgram, aN.LINK_STATUS)
        aX = aN.getProgramParameter(self.shaderProgramOff, aN.LINK_STATUS)
        if not aH or not aX:
            if aH:
                aI = aN.getProgramInfoLog(self.shaderProgram)
            else:
                aI = aN.getProgramInfoLog(self.shaderProgramOff)
            print(f"failed to link program: {aI}")
            if self.vertShader:
                aN.deleteShader(self.vertShader)
                self.vertShader = 0

            if self.fragShader:
                aN.deleteShader(self.fragShader)
                self.fragShader = 0

            if self.shaderProgram:
                aN.deleteProgram(self.shaderProgram)
                self.shaderProgram = 0

            if self.vertShaderOff:
                aN.deleteShader(self.vertShaderOff)
                self.vertShaderOff = 0

            if self.fragShaderOff:
                aN.deleteShader(self.fragShaderOff)
                self.fragShaderOff = 0

            if self.shaderProgramOff:
                aN.deleteProgram(self.shaderProgramOff)
                self.shaderProgramOff = 0

            return False

        return True

    def createFramebuffer(self):
        aL = self.gl
        aK = Live2D.clippingMaskBufferSize
        aJ = aL.createFramebuffer()
        aL.bindFramebuffer(aL.FRAMEBUFFER, aJ)
        aH = aL.createRenderbuffer()
        aL.bindRenderbuffer(aL.RENDERBUFFER, aH)
        aL.renderbufferStorage(aL.RENDERBUFFER, aL.RGBA4, aK, aK)
        aL.framebufferRenderbuffer(aL.FRAMEBUFFER, aL.COLOR_ATTACHMENT0, aL.RENDERBUFFER, aH)
        aI = aL.createTexture()
        aL.bindTexture(aL.TEXTURE_2D, aI)
        aL.texImage2D(aL.TEXTURE_2D, 0, aL.RGBA, aK, aK, 0, aL.RGBA, aL.UNSIGNED_BYTE, None)
        aL.texParameteri(aL.TEXTURE_2D, aL.TEXTURE_MIN_FILTER, aL.LINEAR)
        aL.texParameteri(aL.TEXTURE_2D, aL.TEXTURE_MAG_FILTER, aL.LINEAR)
        aL.texParameteri(aL.TEXTURE_2D, aL.TEXTURE_WRAP_S, aL.CLAMP_TO_EDGE)
        aL.texParameteri(aL.TEXTURE_2D, aL.TEXTURE_WRAP_T, aL.CLAMP_TO_EDGE)
        aL.framebufferTexture2D(aL.FRAMEBUFFER, aL.COLOR_ATTACHMENT0, aL.TEXTURE_2D, aI, 0)
        aL.bindTexture(aL.TEXTURE_2D, 0)
        aL.bindRenderbuffer(aL.RENDERBUFFER, 0)
        aL.bindFramebuffer(aL.FRAMEBUFFER, 0)
        self.framebufferObject = FrameBufferObject(
            fbo=aJ,
            rbo=aH,
            tex=aI
        )


def bindOrCreateVBO(gl, vbo, data):
    if vbo is None:
        vbo = gl.createBuffer()

    gl.bindBuffer(gl.ARRAY_BUFFER, vbo)
    gl.bufferData(gl.ARRAY_BUFFER, data, gl.DYNAMIC_DRAW)
    return vbo


def bindOrCreateEBO(gl, ebo, data):
    if ebo is None:
        ebo = gl.createBuffer()

    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, ebo)
    gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, data, gl.DYNAMIC_DRAW)
    return ebo
