# usage with tkinter

import os.path
import tkinter

import pyautogui
from pyopengltk import OpenGLFrame
import resources

from time import sleep
import live2d.v2 as live2d
# import live2d.v3 as live2d

class AppOgl(OpenGLFrame):
    def __init__(self, master, **kw):
        super().__init__(master, **kw)
        self.model = None

    def initgl(self):
        """Initalize gl states when the frame is created"""
        if self.model:
            del self.model
        live2d.dispose()

        live2d.init()
        live2d.glInit()

        self.model = live2d.LAppModel()
        if live2d.LIVE2D_VERSION == 2:
            self.model.LoadModelJson(os.path.join(resources.RESOURCES_DIRECTORY, "v2/kasumi2/kasumi2.model.json"))
        else:
            self.model.LoadModelJson(os.path.join(resources.RESOURCES_DIRECTORY, "v3/haru/haru.model3.json"))
        self.model.Resize(self.width, self.height)

    def redraw(self):
        """Render a single frame"""
        live2d.clearBuffer()

        screen_x, screen_y = pyautogui.position()
        x = screen_x - self.winfo_rootx()
        y = screen_y - self.winfo_rooty()

        self.model.Update()
        self.model.Drag(x, y)
        self.model.Draw()
        # 控制帧率
        sleep(1 / 60)



if __name__ == '__main__':
    root = tkinter.Tk()
    root.attributes('-transparent', 'black')
    app = AppOgl(root, width=400, height=500)
    root.bind("<Button-1>", lambda _: app.model.StartRandomMotion())
    app.pack(fill=tkinter.BOTH, expand=tkinter.YES)
    app.animate = 1
    app.mainloop()

    live2d.dispose()