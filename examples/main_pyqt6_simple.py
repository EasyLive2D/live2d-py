import os

from PyQt6.QtCore import QTimerEvent, Qt
from PyQt6.QtOpenGLWidgets import QOpenGLWidget
from PyQt6.QtWidgets import QApplication
from OpenGL.GL import *
from PyQt6.QtGui import QSurfaceFormat

import live2d.v3 as live2d
# import live2d.v2 as live2d
import resources
import os
os.environ["QSG_RHI_BACKEND"] = "opengl"

class Win(QOpenGLWidget):

    def __init__(self) -> None:
        super().__init__()
        self.setStyleSheet("background:transparent")
        self.setAttribute(Qt.WidgetAttribute.WA_TranslucentBackground)
        self.setWindowFlags(Qt.WindowType.FramelessWindowHint)

        self.resize(400, 500)

        self.model: live2d.LAppModel | None = None


    def initializeGL(self) -> None:
        # 将当前窗口作为 OpenGL 的上下文
        # 图形会被绘制到当前窗口
        live2d.glInit()
        # 创建模型
        self.model = live2d.LAppModel()

        if live2d.LIVE2D_VERSION == 3:
            self.model.LoadModelJson(os.path.join(resources.RESOURCES_DIRECTORY, "v3/Haru/Haru.model3.json"))
        else:
            self.model.LoadModelJson(os.path.join(resources.RESOURCES_DIRECTORY, "v2/shizuku/shizuku.model.json"))

        # 以 fps = 120 进行绘图
        self.startTimer(int(1000 / 120))

    def resizeGL(self, w: int, h: int) -> None:
        self.model.Resize(w, h)

    def paintGL(self) -> None:
        live2d.clearBuffer(0.7, 0.5, 0.4, 0)
        self.model.Update()

        self.model.Draw()
        
    def mouseMoveEvent(self, event):
        x, y = event.globalPosition().x() - self.x(), event.globalPosition().y() - self.y()
        self.model.Drag(x, y)

    def timerEvent(self, a0: QTimerEvent | None) -> None:
        self.update()


if __name__ == "__main__":
    import sys

    live2d.init()

    app = QApplication(sys.argv) 

    win = Win()

    win.show()
    app.exec()

    live2d.dispose()
