import os
import sys

import resources
from live2d.utils.canvas import Canvas

"""
Example of controlling model opacity using live2d.utils.canvas.Canvas
"""

# import live2d.v2 as live2d


import live2d.v3 as live2d
import math
from PyQt6.QtCore import Qt
from PyQt6.QtOpenGLWidgets import QOpenGLWidget


class Live2DCanvas(QOpenGLWidget):
    def __init__(self):
        super().__init__()
        self.model: None | live2d.LAppModel = None

        # tool for controlling model opacity
        self.canvas: None | Canvas = None

        self.setWindowTitle("Live2DCanvas")
        self.setWindowFlag(Qt.WindowType.FramelessWindowHint)
        self.setAttribute(Qt.WidgetAttribute.WA_TranslucentBackground)
        self.radius_per_frame = math.pi * 0.5 / 120
        self.total_radius = 0

    def initializeGL(self):
        live2d.glInit()
        self.model = live2d.LAppModel()
        if live2d.LIVE2D_VERSION == 3:
            self.model.LoadModelJson(os.path.join(resources.RESOURCES_DIRECTORY, "v3", "llny/llny.model3.json"))
        else:
            self.model.LoadModelJson(os.path.join(resources.RESOURCES_DIRECTORY, "v2", "kasumi2/kasumi2.model.json"))
        
        # must be created after opengl context is configured
        self.canvas = Canvas()

        self.startTimer(int(1000 / 120))
    
    def timerEvent(self, a0):
        self.total_radius += self.radius_per_frame
        v = abs(math.cos(self.total_radius))

        # change opacity
        self.canvas.SetOutputOpacity(v)
        
        self.update()

    def on_draw(self):
        live2d.clearBuffer()
        self.model.Draw()

    def paintGL(self):
        self.model.Update()
        
        # render callback
        self.canvas.Draw(self.on_draw)

    def resizeGL(self, width: int, height: int):
        self.model.Resize(width, height)

        # resize canvas
        self.canvas.SetSize(width, height)


if __name__ == '__main__':
    from PyQt6.QtWidgets import QApplication

    live2d.init()

    app = QApplication(sys.argv)


    win = Live2DCanvas()
    win.show()
    app.exec()
    live2d.dispose()
