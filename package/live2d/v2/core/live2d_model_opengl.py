from typing import TYPE_CHECKING

from .alive2d_model import ALive2DModel

if TYPE_CHECKING:
    from .graphics import GLRenderer


class Live2DModelOpenGL(ALive2DModel):

    def __init__(self, version: str):
        super().__init__()
        self.version = version
        self.drawParamGL = None

    def createRenderer(self):
        from .graphics import GLRenderer
        self.drawParamGL = GLRenderer(self.modelContext, self.version)

    def releaseRenderer(self):
        self.drawParamGL = None

    def resize(self, ww: int, wh: int):
        if self.drawParamGL is not None:
            self.drawParamGL.resize(ww, wh)

    def update(self):
        self.modelContext.update()

    def draw(self):
        if self.drawParamGL is None:
            return
        self.drawParamGL.preDraw(self.modelContext)
        self.drawParamGL.draw(self.modelContext)

    def getDrawParam(self):
        return self.drawParamGL

    def setMatrix(self, aH):
        if self.drawParamGL is not None:
            self.drawParamGL.setMatrix(aH)

    @staticmethod
    def loadModel(aI, version: str, create_renderer: bool = True):
        aH = Live2DModelOpenGL(version)
        ALive2DModel.loadModel_exe(aH, aI)
        if create_renderer:
            aH.createRenderer()
        return aH

    def setTexture(self, aI, aH):
        if self.drawParamGL is None:
            self.createRenderer()

        self.drawParamGL.setTexture(aI, aH)

    def setTextureData(self, no, width, height, data):
        if self.drawParamGL is None:
            self.createRenderer()

        self.drawParamGL.loadTexture(no, width, height, data)
