from typing import List, Any
from typing import TYPE_CHECKING

from .ut_system import UtSystem
if TYPE_CHECKING:
    from ..model_context import ModelContext
    from ..param import PivotManager


class UtInterpolate:
    @staticmethod
    def interpolateInt(mdc: 'ModelContext', pivotMgr: 'PivotManager', ret: List[bool], pivotValue: List[int]):
        a1 = pivotMgr.calcPivotValues(mdc, ret)
        a3 = mdc.getTempPivotTableIndices()
        ba = mdc.getTempT()
        pivotMgr.calcPivotIndices(a3, ba, a1)
        if a1 <= 0:
            return pivotValue[a3[0]]

        elif a1 == 1:
            bj = pivotValue[a3[0]]
            bi = pivotValue[a3[1]]
            a9 = ba[0]
            return int(bj + (bi - bj) * a9)

        elif a1 == 2:
            bj = pivotValue[a3[0]]
            bi = pivotValue[a3[1]]
            a0 = pivotValue[a3[2]]
            aZ = pivotValue[a3[3]]
            a9 = ba[0]
            a8 = ba[1]
            br = int(bj + (bi - bj) * a9)
            bq = int(a0 + (aZ - a0) * a9)
            return int(br + (bq - br) * a8)
        elif a1 == 3:
            aP = pivotValue[a3[0]]
            aO = pivotValue[a3[1]]
            bn = pivotValue[a3[2]]
            bm = pivotValue[a3[3]]
            aK = pivotValue[a3[4]]
            aJ = pivotValue[a3[5]]
            bg = pivotValue[a3[6]]
            bf = pivotValue[a3[7]]
            a9 = ba[0]
            a8 = ba[1]
            a6 = ba[2]
            bj = int(aP + (aO - aP) * a9)
            bi = int(bn + (bm - bn) * a9)
            a0 = int(aK + (aJ - aK) * a9)
            aZ = int(bg + (bf - bg) * a9)
            br = int(bj + (bi - bj) * a8)
            bq = int(a0 + (aZ - a0) * a8)
            return int(br + (bq - br) * a6)
        elif a1 == 4:
            aT = pivotValue[a3[0]]
            aS = pivotValue[a3[1]]
            bu = pivotValue[a3[2]]
            bt = pivotValue[a3[3]]
            aN = pivotValue[a3[4]]
            aM = pivotValue[a3[5]]
            bl = pivotValue[a3[6]]
            bk = pivotValue[a3[7]]
            be = pivotValue[a3[8]]
            bc = pivotValue[a3[9]]
            aX = pivotValue[a3[10]]
            aW = pivotValue[a3[11]]
            a7 = pivotValue[a3[12]]
            a5 = pivotValue[a3[13]]
            aR = pivotValue[a3[14]]
            aQ = pivotValue[a3[15]]
            a9 = ba[0]
            a8 = ba[1]
            a6 = ba[2]
            a4 = ba[3]
            aP = int(aT + (aS - aT) * a9)
            aO = int(bu + (bt - bu) * a9)
            bn = int(aN + (aM - aN) * a9)
            bm = int(bl + (bk - bl) * a9)
            aK = int(be + (bc - be) * a9)
            aJ = int(aX + (aW - aX) * a9)
            bg = int(a7 + (a5 - a7) * a9)
            bf = int(aR + (aQ - aR) * a9)
            bj = int(aP + (aO - aP) * a8)
            bi = int(bn + (bm - bn) * a8)
            a0 = int(aK + (aJ - aK) * a8)
            aZ = int(bg + (bf - bg) * a8)
            br = int(bj + (bi - bj) * a6)
            bq = int(a0 + (aZ - a0) * a6)
            return int(br + (bq - br) * a4)
        else:
            aV = 1 << a1
            aY = [0.0] * (aV)
            for bh in range(0, aV, 1):
                aI: float = bh
                aH = 1
                for aL in range(0, a1, 1):
                    aH *= (1 - ba[aL]) if (aI % 2 == 0) else ba[aL]
                    aI /= 2

                aY[bh] = aH

            bs = [0.0] * (aV)
            for aU in range(0, aV, 1):
                bs[aU] = pivotValue[a3[aU]]

            bd = 0
            for aU in range(0, aV, 1):
                bd += aY[aU] * bs[aU]

            return int(bd + 0.5)

    @staticmethod
    def interpolateFloat(mdc: 'ModelContext', pivotMgr: 'PivotManager', ret: List[bool], pivotValue: List[float]):
        a1 = pivotMgr.calcPivotValues(mdc, ret)
        a2 = mdc.getTempPivotTableIndices()
        a9 = mdc.getTempT()
        pivotMgr.calcPivotIndices(a2, a9, a1)
        if a1 <= 0:
            return pivotValue[a2[0]]

        if a1 == 1:
            bj = pivotValue[a2[0]]
            bi = pivotValue[a2[1]]
            a8 = a9[0]
            return bj + (bi - bj) * a8
        elif a1 == 2:
            bj = pivotValue[a2[0]]
            bi = pivotValue[a2[1]]
            a0 = pivotValue[a2[2]]
            aZ = pivotValue[a2[3]]
            a8 = a9[0]
            a7 = a9[1]
            return (1 - a7) * (bj + (bi - bj) * a8) + a7 * (a0 + (aZ - a0) * a8)
        elif a1 == 3:
            aP = pivotValue[a2[0]]
            aO = pivotValue[a2[1]]
            bn = pivotValue[a2[2]]
            bm = pivotValue[a2[3]]
            aK = pivotValue[a2[4]]
            aJ = pivotValue[a2[5]]
            bf = pivotValue[a2[6]]
            be = pivotValue[a2[7]]
            a8 = a9[0]
            a7 = a9[1]
            a5 = a9[2]
            return (1 - a5) * ((1 - a7) * (aP + (aO - aP) * a8) + a7 * (bn + (bm - bn) * a8)) + a5 * (
                    (1 - a7) * (aK + (aJ - aK) * a8) + a7 * (bf + (be - bf) * a8))
        elif a1 == 4:
            aT = pivotValue[a2[0]]
            aS = pivotValue[a2[1]]
            bs = pivotValue[a2[2]]
            br = pivotValue[a2[3]]
            aN = pivotValue[a2[4]]
            aM = pivotValue[a2[5]]
            bl = pivotValue[a2[6]]
            bk = pivotValue[a2[7]]
            bd = pivotValue[a2[8]]
            bb = pivotValue[a2[9]]
            aX = pivotValue[a2[10]]
            aW = pivotValue[a2[11]]
            a6 = pivotValue[a2[12]]
            a4 = pivotValue[a2[13]]
            aR = pivotValue[a2[14]]
            aQ = pivotValue[a2[15]]
            a8 = a9[0]
            a7 = a9[1]
            a5 = a9[2]
            a3 = a9[3]
            return (1 - a3) * ((1 - a5) * (
                    (1 - a7) * (aT + (aS - aT) * a8) + a7 * (bs + (br - bs) * a8)) + a5 * (
                                       (1 - a7) * (aN + (aM - aN) * a8) + a7 * (
                                       bl + (bk - bl) * a8))) + a3 * ((1 - a5) * (
                    (1 - a7) * (bd + (bb - bd) * a8) + a7 * (aX + (aW - aX) * a8)) + a5 * (
                                                                              (1 - a7) * (
                                                                              a6 + (
                                                                              a4 - a6) * a8) + a7 * (
                                                                                      aR + (
                                                                                      aQ - aR) * a8)))
        else:
            aV = 1 << a1
            aY = [0.0] * (aV)
            for bh in range(0, aV, 1):
                aI: float = bh
                aH = 1
                for aL in range(0, a1, 1):
                    aH *= (1 - a9[aL]) if (aI % 2 == 0) else a9[aL]
                    aI /= 2

                aY[bh] = aH

            bq = [0.0] * (aV)
            for aU in range(0, aV, 1):
                bq[aU] = pivotValue[a2[aU]]

            bc = 0
            for aU in range(0, aV, 1):
                bc += aY[aU] * bq[aU]

            return bc

    @staticmethod
    def interpolatePoints(mdc: 'ModelContext', pivotMgr: 'PivotManager', retParamOut: List[bool], numPts: int, pivotPoints: List[float], dstPoints, ptOffset, ptStep):
        aN = pivotMgr.calcPivotValues(mdc, retParamOut)
        bw = mdc.getTempPivotTableIndices()
        a2 = mdc.getTempT()
        pivotMgr.calcPivotIndices(bw, a2, aN)
        aJ = numPts * 2
        aQ = ptOffset
        if aN <= 0:
            bI = bw[0]
            bq = pivotPoints[bI]
            if ptStep == 2 and ptOffset == 0:
                UtSystem.arraycopy(bq, 0, dstPoints, 0, aJ)
            else:
                bt = 0
                while bt < aJ:
                    dstPoints[aQ] = bq[bt]
                    bt += 1
                    dstPoints[aQ + 1] = bq[bt]
                    bt += 1
                    aQ += ptStep
        elif aN == 1:
            bq = pivotPoints[bw[0]]
            bp = pivotPoints[bw[1]]
            b3 = a2[0]
            bT = 1 - b3
            bt = 0
            while bt < aJ:
                dstPoints[aQ] = bq[bt] * bT + bp[bt] * b3
                bt += 1
                dstPoints[aQ + 1] = bq[bt] * bT + bp[bt] * b3
                bt += 1
                aQ += ptStep
        elif aN == 2:
            bq = pivotPoints[bw[0]]
            bp = pivotPoints[bw[1]]
            aZ = pivotPoints[bw[2]]
            aY = pivotPoints[bw[3]]
            b3 = a2[0]
            b1 = a2[1]
            bT = 1 - b3
            bP = 1 - b1
            b2 = bP * bT
            b0 = bP * b3
            bM = b1 * bT
            bL = b1 * b3
            bt = 0
            while bt < aJ:
                dstPoints[aQ] = b2 * bq[bt] + b0 * bp[bt] + bM * aZ[bt] + bL * aY[bt]
                bt += 1
                dstPoints[aQ + 1] = b2 * bq[bt] + b0 * bp[bt] + bM * aZ[bt] + bL * aY[bt]
                bt += 1
                aQ += ptStep
        elif aN == 3:
            ba = pivotPoints[bw[0]]
            a9 = pivotPoints[bw[1]]
            aP = pivotPoints[bw[2]]
            aO = pivotPoints[bw[3]]
            a6 = pivotPoints[bw[4]]
            a4 = pivotPoints[bw[5]]
            aL = pivotPoints[bw[6]]
            aK = pivotPoints[bw[7]]
            b3 = a2[0]
            b1 = a2[1]
            bZ = a2[2]
            bT = 1 - b3
            bP = 1 - b1
            bN = 1 - bZ
            b8 = bN * bP * bT
            b7 = bN * bP * b3
            bU = bN * b1 * bT
            bS = bN * b1 * b3
            b6 = bZ * bP * bT
            b5 = bZ * bP * b3
            bQ = bZ * b1 * bT
            bO = bZ * b1 * b3
            bt = 0
            while bt < aJ:
                dstPoints[aQ] = b8 * ba[bt] + b7 * a9[bt] + bU * aP[bt] + bS * aO[bt] + b6 * a6[bt] + b5 * a4[
                    bt] + bQ * aL[bt] + bO * aK[bt]
                bt += 1
                dstPoints[aQ + 1] = b8 * ba[bt] + b7 * a9[bt] + bU * aP[bt] + bS * aO[bt] + b6 * a6[bt] + b5 * a4[
                    bt] + bQ * aL[bt] + bO * aK[bt]
                bt += 1
                aQ += ptStep
        elif aN == 4:
            bD = pivotPoints[bw[0]]
            bB = pivotPoints[bw[1]]
            bo = pivotPoints[bw[2]]
            bm = pivotPoints[bw[3]]
            by = pivotPoints[bw[4]]
            bx = pivotPoints[bw[5]]
            be = pivotPoints[bw[6]]
            bd = pivotPoints[bw[7]]
            bG = pivotPoints[bw[8]]
            bE = pivotPoints[bw[9]]
            bv = pivotPoints[bw[10]]
            bu = pivotPoints[bw[11]]
            bA = pivotPoints[bw[12]]
            bz = pivotPoints[bw[13]]
            bn = pivotPoints[bw[14]]
            bl = pivotPoints[bw[15]]
            b3 = a2[0]
            b1 = a2[1]
            bZ = a2[2]
            bY = a2[3]
            bT = 1 - b3
            bP = 1 - b1
            bN = 1 - bZ
            bK = 1 - bY
            bk = bK * bN * bP * bT
            bi = bK * bN * bP * b3
            aW = bK * bN * b1 * bT
            aV = bK * bN * b1 * b3
            bc = bK * bZ * bP * bT
            bb = bK * bZ * bP * b3
            aS = bK * bZ * b1 * bT
            aR = bK * bZ * b1 * b3
            bs = bY * bN * bP * bT
            br = bY * bN * bP * b3
            a1 = bY * bN * b1 * bT
            a0 = bY * bN * b1 * b3
            bh = bY * bZ * bP * bT
            bf = bY * bZ * bP * b3
            aU = bY * bZ * b1 * bT
            aT = bY * bZ * b1 * b3
            bt = 0
            while bt < aJ:
                dstPoints[aQ] = bk * bD[bt] + bi * bB[bt] + aW * bo[bt] + aV * bm[bt] + bc * by[bt] + bb * bx[
                    bt] + aS * be[bt] + aR * bd[bt] + bs * bG[bt] + br * bE[bt] + a1 * bv[bt] + a0 * bu[
                                    bt] + bh * bA[bt] + bf * bz[bt] + aU * bn[bt] + aT * bl[bt]
                bt += 1
                dstPoints[aQ + 1] = bk * bD[bt] + bi * bB[bt] + aW * bo[bt] + aV * bm[bt] + bc * by[bt] + bb * \
                                    bx[bt] + aS * be[bt] + aR * bd[bt] + bs * bG[bt] + br * bE[bt] + a1 * bv[
                                        bt] + a0 * bu[bt] + bh * bA[bt] + bf * bz[bt] + aU * bn[bt] + aT * bl[
                                        bt]
                bt += 1
                aQ += ptStep
        else:
            b4 = 1 << aN
            bJ = [0.0] * (b4)
            for bj in range(0, b4, 1):
                aH: float = bj
                aM = 1
                for bF in range(0, aN, 1):
                    aM *= (1 - a2[bF]) if (aH % 2 == 0) else a2[bF]
                    aH /= 2

                bJ[bj] = aM

            bg: List[List[float]] | List[Any] = [0.0] * (b4)
            for aX in range(0, b4, 1):
                bg[aX] = pivotPoints[bw[aX]]

            bt = 0
            while bt < aJ:
                a8 = 0
                a7 = 0
                bR = bt + 1
                for aX in range(0, b4, 1):
                    a8 += bJ[aX] * bg[aX][bt]
                    a7 += bJ[aX] * bg[aX][bR]

                bt += 2
                dstPoints[aQ] = a8
                dstPoints[aQ + 1] = a7
                aQ += ptStep
