from .deformer import Deformer
from .warp_context import WarpContext
from ..param import PivotManager
from ..util import UtInterpolate


class WarpDeformer(Deformer):
    paramOutSide = [False]

    def __init__(self):
        super().__init__()
        self.row = 0
        self.col = 0
        self.pivotMgr = None
        self.pivotPoints = None

    def read(self, br):
        super().read(br)
        self.col = br.readInt32()
        self.row = br.readInt32()
        self.pivotMgr = br.readObject()
        self.pivotPoints = br.readObject()
        super().readOpacity(br)

    def init(self, mc):
        aI = WarpContext(self)
        aJ = (self.row + 1) * (self.col + 1)
        if aI.interpolatedPoints is not None:
            aI.interpolatedPoints = None

        aI.interpolatedPoints = [0.0] * (aJ * 2)
        if aI.transformedPoints is not None:
            aI.transformedPoints = None

        if self.needTransform():
            aI.transformedPoints = [0.0] * (aJ * 2)
        else:
            aI.transformedPoints = None

        return aI

    def setupInterpolate(self, modelContext, deformerContext: 'WarpContext'):
        aK = deformerContext
        if not self.pivotMgr.checkParamUpdated(modelContext):
            return

        aL = self.getPointCount()
        aH = WarpDeformer.paramOutSide
        aH[0] = False
        UtInterpolate.interpolatePoints(modelContext, self.pivotMgr, aH, aL, self.pivotPoints, aK.interpolatedPoints, 0,
                                        2)
        deformerContext.setOutsideParam(aH[0])
        self.interpolateOpacity(modelContext, self.pivotMgr, deformerContext, aH)

    def setupTransform(self, modelContext, deformerContext):
        aL = deformerContext
        aL.setAvailable(True)
        if not self.needTransform():
            aL.setTotalOpacity(aL.getInterpolatedOpacity())
        else:
            aH = self.getTargetId()
            if aL.tmpDeformerIndex == Deformer.DEFORMER_INDEX_NOT_INIT:
                aL.tmpDeformerIndex = modelContext.getDeformerIndex(aH)

            if aL.tmpDeformerIndex < 0:
                print("deformer is not reachable")

                aL.setAvailable(False)
            else:
                aN = modelContext.getDeformer(aL.tmpDeformerIndex)
                aI = modelContext.getDeformerContext(aL.tmpDeformerIndex)
                if aN is not None and aI.isAvailable():
                    aM = aI.getTotalScale()
                    aL.setTotalScale_notForClient(aM)
                    aO = aI.getTotalOpacity()
                    aL.setTotalOpacity(aO * aL.getInterpolatedOpacity())
                    aN.transformPoints(modelContext, aI, aL.interpolatedPoints, aL.transformedPoints, self.getPointCount(), 0, 2)
                    aL.setAvailable(True)
                else:
                    aL.setAvailable(False)

    def transformPoints(self, mc, dc: 'WarpContext', srcPoints, dstPoints, numPoint, ptOffset, ptStep):
        pivot_points = dc.transformedPoints if (dc.transformedPoints is not None) else dc.interpolatedPoints
        WarpDeformer.transformPoints_sdk2(srcPoints, dstPoints, numPoint, ptOffset, ptStep, pivot_points, self.row,
                                          self.col)

    def getPointCount(self):
        return (self.row + 1) * (self.col + 1)

    def getType(self):
        return Deformer.TYPE_WARP

    @staticmethod
    def transformPoints_sdk2(hvs, dst, pointCount, srcOffset, srcStep, grid, row, col):
        aW = pointCount * srcStep
        aT = 0
        aS = 0
        bl = 0
        bk = 0
        bf = 0
        be = 0
        aZ = False
        for ba in range(srcOffset, aW, srcStep):
            a4 = hvs[ba]
            aX = hvs[ba + 1]
            bd = a4 * row
            a7 = aX * col
            if bd < 0 or a7 < 0 or row <= bd or col <= a7:
                a1 = row + 1
                if not aZ:
                    aZ = True
                    aT = 0.25 * (grid[((0) + (0) * a1) * 2] + grid[((row) + (0) * a1) * 2] + grid[
                        ((0) + (col) * a1) * 2] +
                                 grid[((row) + (col) * a1) * 2])
                    aS = 0.25 * (grid[((0) + (0) * a1) * 2 + 1] + grid[((row) + (0) * a1) * 2 + 1] + grid[
                        ((0) + (col) * a1) * 2 + 1] + grid[((row) + (col) * a1) * 2 + 1])
                    aM = grid[((row) + (col) * a1) * 2] - grid[((0) + (0) * a1) * 2]
                    aL = grid[((row) + (col) * a1) * 2 + 1] - grid[((0) + (0) * a1) * 2 + 1]
                    bh = grid[((row) + (0) * a1) * 2] - grid[((0) + (col) * a1) * 2]
                    bg = grid[((row) + (0) * a1) * 2 + 1] - grid[((0) + (col) * a1) * 2 + 1]
                    bl = (aM + bh) * 0.5
                    bk = (aL + bg) * 0.5
                    bf = (aM - bh) * 0.5
                    be = (aL - bg) * 0.5
                    aT -= 0.5 * (bl + bf)
                    aS -= 0.5 * (bk + be)

                if (-2 < a4 and a4 < 3) and (-2 < aX and aX < 3):
                    if a4 <= 0:
                        if aX <= 0:
                            a3 = grid[((0) + (0) * a1) * 2]
                            a2 = grid[((0) + (0) * a1) * 2 + 1]
                            a8 = aT - 2 * bl
                            a6 = aS - 2 * bk
                            aK = aT - 2 * bf
                            aJ = aS - 2 * be
                            aO = aT - 2 * bl - 2 * bf
                            aN = aS - 2 * bk - 2 * be
                            bj = 0.5 * (a4 - (-2))
                            bi = 0.5 * (aX - (-2))
                            if bj + bi <= 1:
                                dst[ba] = aO + (aK - aO) * bj + (a8 - aO) * bi
                                dst[ba + 1] = aN + (aJ - aN) * bj + (a6 - aN) * bi
                            else:
                                dst[ba] = a3 + (a8 - a3) * (1 - bj) + (aK - a3) * (1 - bi)
                                dst[ba + 1] = a2 + (a6 - a2) * (1 - bj) + (aJ - a2) * (1 - bi)
                        else:
                            if aX >= 1:
                                aK = grid[((0) + (col) * a1) * 2]
                                aJ = grid[((0) + (col) * a1) * 2 + 1]
                                aO = aT - 2 * bl + 1 * bf
                                aN = aS - 2 * bk + 1 * be
                                a3 = aT + 3 * bf
                                a2 = aS + 3 * be
                                a8 = aT - 2 * bl + 3 * bf
                                a6 = aS - 2 * bk + 3 * be
                                bj = 0.5 * (a4 - (-2))
                                bi = 0.5 * (aX - (1))
                                if bj + bi <= 1:
                                    dst[ba] = aO + (aK - aO) * bj + (a8 - aO) * bi
                                    dst[ba + 1] = aN + (aJ - aN) * bj + (a6 - aN) * bi
                                else:
                                    dst[ba] = a3 + (a8 - a3) * (1 - bj) + (aK - a3) * (1 - bi)
                                    dst[ba + 1] = a2 + (a6 - a2) * (1 - bj) + (aJ - a2) * (1 - bi)
                            else:
                                aH = int(a7)
                                if aH == col:
                                    aH = col - 1

                                bj = 0.5 * (a4 - (-2))
                                bi = a7 - aH
                                bb = aH / col
                                a9 = (aH + 1) / col
                                aK = grid[((0) + (aH) * a1) * 2]
                                aJ = grid[((0) + (aH) * a1) * 2 + 1]
                                a3 = grid[((0) + (aH + 1) * a1) * 2]
                                a2 = grid[((0) + (aH + 1) * a1) * 2 + 1]
                                aO = aT - 2 * bl + bb * bf
                                aN = aS - 2 * bk + bb * be
                                a8 = aT - 2 * bl + a9 * bf
                                a6 = aS - 2 * bk + a9 * be
                                if bj + bi <= 1:
                                    dst[ba] = aO + (aK - aO) * bj + (a8 - aO) * bi
                                    dst[ba + 1] = aN + (aJ - aN) * bj + (a6 - aN) * bi
                                else:
                                    dst[ba] = a3 + (a8 - a3) * (1 - bj) + (aK - a3) * (1 - bi)
                                    dst[ba + 1] = a2 + (a6 - a2) * (1 - bj) + (aJ - a2) * (1 - bi)
                    else:
                        if 1 <= a4:
                            if aX <= 0:
                                a8 = grid[((row) + (0) * a1) * 2]
                                a6 = grid[((row) + (0) * a1) * 2 + 1]
                                a3 = aT + 3 * bl
                                a2 = aS + 3 * bk
                                aO = aT + 1 * bl - 2 * bf
                                aN = aS + 1 * bk - 2 * be
                                aK = aT + 3 * bl - 2 * bf
                                aJ = aS + 3 * bk - 2 * be
                                bj = 0.5 * (a4 - (1))
                                bi = 0.5 * (aX - (-2))
                                if bj + bi <= 1:
                                    dst[ba] = aO + (aK - aO) * bj + (a8 - aO) * bi
                                    dst[ba + 1] = aN + (aJ - aN) * bj + (a6 - aN) * bi
                                else:
                                    dst[ba] = a3 + (a8 - a3) * (1 - bj) + (aK - a3) * (1 - bi)
                                    dst[ba + 1] = a2 + (a6 - a2) * (1 - bj) + (aJ - a2) * (1 - bi)
                            else:
                                if aX >= 1:
                                    aO = grid[((row) + (col) * a1) * 2]
                                    aN = grid[((row) + (col) * a1) * 2 + 1]
                                    aK = aT + 3 * bl + 1 * bf
                                    aJ = aS + 3 * bk + 1 * be
                                    a8 = aT + 1 * bl + 3 * bf
                                    a6 = aS + 1 * bk + 3 * be
                                    a3 = aT + 3 * bl + 3 * bf
                                    a2 = aS + 3 * bk + 3 * be
                                    bj = 0.5 * (a4 - (1))
                                    bi = 0.5 * (aX - (1))
                                    if bj + bi <= 1:
                                        dst[ba] = aO + (aK - aO) * bj + (a8 - aO) * bi
                                        dst[ba + 1] = aN + (aJ - aN) * bj + (a6 - aN) * bi
                                    else:
                                        dst[ba] = a3 + (a8 - a3) * (1 - bj) + (aK - a3) * (1 - bi)
                                        dst[ba + 1] = a2 + (a6 - a2) * (1 - bj) + (aJ - a2) * (1 - bi)
                                else:
                                    aH = int(a7)
                                    if aH == col:
                                        aH = col - 1

                                    bj = 0.5 * (a4 - (1))
                                    bi = a7 - aH
                                    bb = aH / col
                                    a9 = (aH + 1) / col
                                    aO = grid[((row) + (aH) * a1) * 2]
                                    aN = grid[((row) + (aH) * a1) * 2 + 1]
                                    a8 = grid[((row) + (aH + 1) * a1) * 2]
                                    a6 = grid[((row) + (aH + 1) * a1) * 2 + 1]
                                    aK = aT + 3 * bl + bb * bf
                                    aJ = aS + 3 * bk + bb * be
                                    a3 = aT + 3 * bl + a9 * bf
                                    a2 = aS + 3 * bk + a9 * be
                                    if bj + bi <= 1:
                                        dst[ba] = aO + (aK - aO) * bj + (a8 - aO) * bi
                                        dst[ba + 1] = aN + (aJ - aN) * bj + (a6 - aN) * bi
                                    else:
                                        dst[ba] = a3 + (a8 - a3) * (1 - bj) + (aK - a3) * (1 - bi)
                                        dst[ba + 1] = a2 + (a6 - a2) * (1 - bj) + (aJ - a2) * (1 - bi)
                        else:
                            if aX <= 0:
                                aY = int(bd)
                                if aY == row:
                                    aY = row - 1

                                bj = bd - aY
                                bi = 0.5 * (aX - (-2))
                                bp = aY / row
                                bo = (aY + 1) / row
                                a8 = grid[((aY) + (0) * a1) * 2]
                                a6 = grid[((aY) + (0) * a1) * 2 + 1]
                                a3 = grid[((aY + 1) + (0) * a1) * 2]
                                a2 = grid[((aY + 1) + (0) * a1) * 2 + 1]
                                aO = aT + bp * bl - 2 * bf
                                aN = aS + bp * bk - 2 * be
                                aK = aT + bo * bl - 2 * bf
                                aJ = aS + bo * bk - 2 * be
                                if bj + bi <= 1:
                                    dst[ba] = aO + (aK - aO) * bj + (a8 - aO) * bi
                                    dst[ba + 1] = aN + (aJ - aN) * bj + (a6 - aN) * bi
                                else:
                                    dst[ba] = a3 + (a8 - a3) * (1 - bj) + (aK - a3) * (1 - bi)
                                    dst[ba + 1] = a2 + (a6 - a2) * (1 - bj) + (aJ - a2) * (1 - bi)
                            else:
                                if aX >= 1:
                                    aY = int(bd)
                                    if aY == row:
                                        aY = row - 1

                                    bj = bd - aY
                                    bi = 0.5 * (aX - (1))
                                    bp = aY / row
                                    bo = (aY + 1) / row
                                    aO = grid[((aY) + (col) * a1) * 2]
                                    aN = grid[((aY) + (col) * a1) * 2 + 1]
                                    aK = grid[((aY + 1) + (col) * a1) * 2]
                                    aJ = grid[((aY + 1) + (col) * a1) * 2 + 1]
                                    a8 = aT + bp * bl + 3 * bf
                                    a6 = aS + bp * bk + 3 * be
                                    a3 = aT + bo * bl + 3 * bf
                                    a2 = aS + bo * bk + 3 * be
                                    if bj + bi <= 1:
                                        dst[ba] = aO + (aK - aO) * bj + (a8 - aO) * bi
                                        dst[ba + 1] = aN + (aJ - aN) * bj + (a6 - aN) * bi
                                    else:
                                        dst[ba] = a3 + (a8 - a3) * (1 - bj) + (aK - a3) * (1 - bi)
                                        dst[ba + 1] = a2 + (a6 - a2) * (1 - bj) + (aJ - a2) * (1 - bi)
                                else:
                                    raise RuntimeError("error @BDBoxGrid")
                else:
                    dst[ba] = aT + a4 * bl + aX * bf
                    dst[ba + 1] = aS + a4 * bk + aX * be
            else:
                bn = bd - int(bd)
                bm = a7 - int(a7)
                aV = 2 * (int(bd) + int(a7) * (row + 1))
                if bn + bm < 1:
                    dst[ba] = grid[aV] * (1 - bn - bm) + grid[aV + 2] * bn + grid[aV + 2 * (row + 1)] * bm
                    dst[ba + 1] = grid[aV + 1] * (1 - bn - bm) + grid[aV + 3] * bn + grid[aV + 2 * (row + 1) + 1] * bm
                else:
                    dst[ba] = grid[aV + 2 * (row + 1) + 2] * (bn - 1 + bm) + grid[aV + 2 * (row + 1)] * (1 - bn) + grid[
                        aV + 2] * (1 - bm)
                    dst[ba + 1] = grid[aV + 2 * (row + 1) + 3] * (bn - 1 + bm) + grid[aV + 2 * (row + 1) + 1] * (
                            1 - bn) + \
                                  grid[aV + 3] * (1 - bm)
