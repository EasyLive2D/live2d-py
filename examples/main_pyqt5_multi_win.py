# example of rendering live2d in multiple opengl windows at one time

import live2d.v3 as live2d

from PyQt5.Qt import QCursor
from PyQt5.QtWidgets import QOpenGLWidget

import resources
import os

class ClonePet(QOpenGLWidget):
    def __init__(self):
        super().__init__()
        self.setFixedSize(400, 400)

        self.pet_model: live2d.LAppModel | None = None

    def timerEvent(self, a0):
        x, y = QCursor.pos().x() - self.x(), QCursor.pos().y() - self.y()
        self.pet_model.Drag(x, y)

        self.update()

    def initializeGL(self):
        live2d.glInit()
        self.pet_model = live2d.LAppModel()
        self.pet_model.LoadModelJson(os.path.join(resources.RESOURCES_DIRECTORY, "v3/Haru/Haru.model3.json"))
        self.startTimer(1)

    def resizeGL(self, w, h):
        self.pet_model.Resize(w, h)

    def paintGL(self):
        live2d.clearBuffer()
        self.pet_model.Update()
        self.pet_model.Draw()


live2d.init()
if __name__ == '__main__':
    import sys
    from PyQt5.QtWidgets import QApplication
    from PyQt5.QtCore import QCoreApplication, Qt

    QCoreApplication.setAttribute(Qt.ApplicationAttribute.AA_ShareOpenGLContexts)
    app = QApplication(sys.argv)
    window = ClonePet()
    window.show()
    window2 = ClonePet()
    window2.show()
    sys.exit(app.exec_())