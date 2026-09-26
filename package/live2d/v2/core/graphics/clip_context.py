from .clip_rectf import ClipRectF
from .clip_draw_context import ClipDrawContext


class ClipContext:

    def __init__(self, aH, aK, aI):
        self.clipIDList = []
        self.clipIDList = aI
        self.clippingMaskDrawIndexList = []
        for aJ in range(0, len(aI), 1):
            self.clippingMaskDrawIndexList.append(aK.getDrawDataIndex(aI[aJ]))

        self.clippedDrawContextList = []
        self.isUsing = True
        self.layoutChannelNo = 0
        self.layoutBounds = ClipRectF()
        self.allClippedDrawRect = ClipRectF()
        self.matrixForMask = [0.0] * (16)
        self.matrixForDraw = [0.0] * (16)
        self.owner = aH

    def addClippedDrawData(self, aJ, aI):
        aH = ClipDrawContext(aJ, aI)
        self.clippedDrawContextList.append(aH)
