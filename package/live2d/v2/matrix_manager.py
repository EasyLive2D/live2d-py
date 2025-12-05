import math
from typing import Tuple, TYPE_CHECKING

from .framework import L2DMatrix44

if TYPE_CHECKING:
    from .framework.matrix import L2DModelMatrix


class MatrixManager:

    def __init__(self):
        self.__projection = L2DMatrix44()
        self.__screenToScene = L2DMatrix44()
        self.__ww = 600
        self.__wh = 600
        self.__rotation = [
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        ]
        self.__offsetX = 0
        self.__offsetY = 0
        self.__scale = 1

    def getWidth(self):
        return self.__ww

    def getHeight(self):
        return self.__wh

    def onResize(self, width: int, height: int):
        self.__ww = width
        self.__wh = height

        ratio = float(width) / float(height)
        left = -ratio
        right = ratio
        bottom = -1.0
        top = 1.0

        self.__screenToScene.identity()
        self.__screenToScene.multTranslate(-width / 2, -height / 2)

        if width > height:
            sw = abs(right - left)
            self.__screenToScene.multScale(sw / width, -sw / width)
        else:
            sh = abs(top - bottom)
            self.__screenToScene.multScale(sh / height, -sh / height)

    def screenToScene(self, scr_x: float, scr_y: float) -> Tuple[float, float]:
        return self.__screenToScene.transformX(scr_x), self.__screenToScene.transformY(scr_y)

    def invertTransform(self, src_x, src_y) -> Tuple[float, float]:
        return self.__projection.invertTransformX(src_x), self.__projection.invertTransformY(src_y)

    def setScale(self, scale: float):
        self.__scale = scale

    def setOffset(self, dx: float, dy: float):
        self.__offsetX = dx
        self.__offsetY = dy

    def getMvp(self, model_matrix: 'L2DModelMatrix') -> list:
        self.__projection.identity()

        if self.__wh > self.__ww:
            model_matrix.setWidth(2.0)
            self.__projection.multScale(1.0, self.__ww / self.__wh)
        else:
            self.__projection.multScale(self.__wh / self.__ww, 1.0)

        self.__projection.multScale(self.__scale, self.__scale)
        self.__projection.translate(self.__offsetX, self.__offsetY)

        self.__projection.mul(self.__projection.getArray(), model_matrix.getArray(), self.__projection.getArray())
        self.__projection.mul(self.__rotation, self.__projection.getArray(), self.__projection.getArray())
        return self.__projection.getArray()

    def rotate(self, deg: float):
        r = deg / 180.0 * math.pi
        self.__rotation = [
            math.cos(r), math.sin(r), 0, 0,
            -math.sin(r), math.cos(r), 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1,
        ]