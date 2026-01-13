# usage with moderngl

import sys
import numpy as np
import moderngl

from PySide6.QtWidgets import QApplication
from PySide6.QtOpenGLWidgets import QOpenGLWidget

import live2d.v3 as live2d
# import live2d.v2 as live2d
import os
import OpenGL.GL as GL

import resources

from typing import Optional


class DefaultRenderer:

    _instance = None

    vertex = """
    #version 330 core
    in vec2 in_position;
    in vec3 in_color;
    out vec3 v_color;
    void main() 
    {
        gl_Position = vec4(in_position, 0.0, 1.0);
        v_color = in_color;
    }
    """

    fragment = """
    #version 330 core
    in vec3 v_color;
    out vec4 FragColor;
    void main() 
    {
        FragColor = vec4(v_color, 1.0);
    }
    """
    vao: moderngl.VertexArray
    prog: moderngl.Program

    def __init__(self, ctx: 'moderngl.Context'):
        self._ctx = ctx

    def __new__(cls, ctx: 'moderngl.Context'):
        if cls._instance is None:
            cls._instance = object.__new__(cls)
            setattr(cls._instance, "prog", ctx.program(cls.vertex, cls.fragment))
            vbo = np.array([
                # 位置        # 颜色
                0.0, 1.0,     1.0, 0.0, 0.0,  # 上顶点 - 红色
                -1.0, -1.0,   0.0, 1.0, 0.0,  # 左下顶点 - 绿色
                1.0, -1.0,    0.0, 0.0, 1.0   # 右下顶点 - 蓝色
            ], dtype=np.float32)
            data = ctx.buffer(vbo.tobytes())
            setattr(cls._instance, "vao", ctx.vertex_array(cls._instance.prog, data, "in_position", "in_color"))
            return cls._instance
    
    def Render(self):
        self.vao.render(moderngl.TRIANGLES)

class GLWidget(QOpenGLWidget):
    def __init__(self, parent=None):
        super().__init__()
        
        # 初始化变量
        self.ctx: Optional[moderngl.Context] = None
        self.model: Optional[live2d.LAppModel] = None
        self.triangle: Optional[DefaultRenderer] = None
        
        # 设置窗口尺寸
        self.setMinimumSize(800, 600)
    
    def initializeGL(self):
        # 创建ModernGL上下文
        self.ctx = moderngl.create_context()

        live2d.glInit()

        self.model = live2d.LAppModel()
        self.model.LoadModelJson(
            os.path.join(
                resources.RESOURCES_DIRECTORY, 
                "v3/llny/llny.model3.json" if live2d.LIVE2D_VERSION == 3
                         else "v2/kasumi2/kasumi2.model.json"
                         ))
        
        self.triangle = DefaultRenderer(self.ctx)

        self.first = False

        self.startTimer(int(1000 / 60))

    def timerEvent(self, e):
        self.update()

    def paintGL(self):
        fbo = self.ctx.detect_framebuffer()
        fbo.use()

        # 清除屏幕
        self.ctx.clear(0.2, 0.2, 0.2, 1.0)

        self.triangle.Render()

        GL.glBindVertexArray(0) # 绘制live2d 前 解绑 vao
        self.model.Update()
        self.model.Draw()


    def resizeGL(self, w, h):
        self.ctx.viewport = (0, 0, w, h)
        self.model.Resize(w, h)


if __name__ == '__main__':
    live2d.init()
    app = QApplication(sys.argv)
    window = GLWidget()
    window.setWindowTitle('ModernGL + PySide6 + live2d')
    window.show()
    app.exec()
    live2d.dispose()