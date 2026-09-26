from typing import TYPE_CHECKING, Dict, Union, Optional

from PIL import Image

from ...core import Live2DMotion
from ...core.live2d_model_opengl import Live2DModelOpenGL
from ..matrix import L2DModelMatrix
from ..motion import L2DExpressionMotion, L2DMotionManager
from ..physics import L2DPhysics
from ..pose import L2DPose


class L2DBaseModel:
    texCount = 0

    def __init__(self):
        self.live2DModel: Optional[Live2DModelOpenGL] = None
        self.modelMatrix: Optional[L2DModelMatrix] = None
        self.eyeBlink = None
        self.physics = None
        self.pose: Union[None, L2DPose] = None
        self.debugMode = False
        self.initialized = False
        self.updating = False
        self.alpha = 1
        self.accAlpha = 0
        self.accelX = 0
        self.accelY = 0
        self.accelZ = 0
        self.dragX = 0
        self.dragY = 0
        self.startTimeMSec = 0
        self.mainMotionManager = L2DMotionManager()
        self.expressionManager = L2DMotionManager()
        self.motions = {}
        self.expressions: Dict = {}
        self.isTexLoaded = False
        self._pendingTextures = []

    def getModelMatrix(self):
        return self.modelMatrix

    def setAlpha(self, alpha):
        if alpha > 0.999:
            alpha = 1
        if alpha < 0.001:
            alpha = 0
        self.alpha = alpha

    def getAlpha(self):
        return self.alpha

    def isInitialized(self):
        return self.initialized

    def setInitialized(self, value):
        self.initialized = value

    def isUpdating(self):
        return self.updating

    def setUpdating(self, value):
        self.updating = value

    def getLive2DModel(self):
        return self.live2DModel

    def setLipSync(self, value):
        self.lipSync = value

    def setLipSyncValue(self, value):
        self.lipSyncValue = value

    def setAccel(self, x, y, z):
        self.accelX = x
        self.accelY = y
        self.accelZ = z

    def setDrag(self, x, y):
        self.dragX = x
        self.dragY = y

    def getMainMotionManager(self):
        return self.mainMotionManager

    def getExpressionManager(self):
        return self.expressionManager

    def loadModelData(self, path, version: str, create_renderer: bool = True):
        with open(path, 'rb') as f:
            self.live2DModel = Live2DModelOpenGL.loadModel(f.read(), version, create_renderer)
        self.live2DModel.saveParam()

        self.modelMatrix = L2DModelMatrix(self.live2DModel.getCanvasWidth(),
                                          self.live2DModel.getCanvasHeight())
        self.modelMatrix.setWidth(2)
        self.modelMatrix.setCenterPosition(0, 0)

        return self.live2DModel

    def loadTexture(self, no, path):
        self.texCount += 1
        if self.live2DModel.getDrawParam() is None:
            self._pendingTextures.append((no, path))
        else:
            self._uploadTexture(no, path)

        self.texCount -= 1
        if self.texCount == 0:
            self.isTexLoaded = True

    def _uploadTexture(self, no, path):
        image = Image.open(path)
        if image.mode != 'RGBA':
            image = image.convert("RGBA")
        width, height = image.size
        self.live2DModel.setTextureData(no, width, height, image.tobytes())

    def flushPendingTextures(self):
        pending = self._pendingTextures
        self._pendingTextures = []
        for no, path in pending:
            self._uploadTexture(no, path)

    def loadMotion(self, name, path):
        with open(path, 'rb') as f:
            buf = f.read()

        motion = Live2DMotion.loadMotion(buf)
        if name is not None:
            self.motions[name] = motion
        return motion

    def loadExpression(self, name, path):
        if name is not None:
            with open(path, 'rb') as f:
                buf = f.read()
            self.expressions[name] = L2DExpressionMotion.loadJson(buf)

    def loadPose(self, path) -> L2DPose:
        with open(path, 'rb') as f:
            buf = f.read()
        self.pose = L2DPose.load(buf)
        return self.pose

    def loadPhysics(self, path):
        with open(path, 'rb') as f:
            buf = f.read()
        self.physics = L2DPhysics.load(buf)

    def hitTestSimple(self, drawID, testX, testY):
        draw_index = self.live2DModel.getDrawDataIndex(drawID)
        if draw_index < 0:
            return False
        points = self.live2DModel.getTransformedPoints(draw_index)
        left = self.live2DModel.getCanvasWidth()
        right = 0
        top = self.live2DModel.getCanvasHeight()
        bottom = 0
        for j in range(0, len(points), 2):
            x = points[j]
            y = points[j + 1]
            if x < left:
                left = x
            if x > right:
                right = x
            if y < top:
                top = y
            if y > bottom:
                bottom = y

        tx = self.modelMatrix.invertTransformX(testX)
        ty = self.modelMatrix.invertTransformY(testY)
        return left <= tx <= right and top <= ty <= bottom
