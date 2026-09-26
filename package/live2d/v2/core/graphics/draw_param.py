from abc import ABC, abstractmethod


class DrawParam(ABC):

    def __init__(self):
        self.baseAlpha = 1
        self.baseRed = 1
        self.baseGreen = 1
        self.baseBlue = 1
        self.culling = False
        self.matrix4x4 = [0.0] * 16
        self.clipBufPre_clipContextMask = None
        self.clipBufPre_clipContextDraw = None
        self.channel_colors = {}

    def setChannelFlagAsColor(self, aH, aI):
        self.channel_colors[aH] = aI

    def getChannelFlagAsColor(self, aY):
        return self.channel_colors[aY]

    @abstractmethod
    def setupDraw(self):
        pass

    @abstractmethod
    def drawTexture(self, texNo, screenColor, indexArray, vertexArray, uvArray, opacity, comp, multiplyColor):
        pass

    def setCulling(self, aH):
        self.culling = aH

    def setMatrix(self, aH):
        for aI in range(0, 16, 1):
            self.matrix4x4[aI] = aH[aI]

    def getMatrix(self):
        return self.matrix4x4

    def setClipBufPre_clipContextForMask(self, aH):
        self.clipBufPre_clipContextMask = aH

    def getClipBufPre_clipContextMask(self):
        return self.clipBufPre_clipContextMask

    def setClipBufPre_clipContextForDraw(self, aH):
        self.clipBufPre_clipContextDraw = aH

    def getClipBufPre_clipContextDraw(self):
        return self.clipBufPre_clipContextDraw
