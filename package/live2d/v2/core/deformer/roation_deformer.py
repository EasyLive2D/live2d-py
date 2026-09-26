import math
from typing import TYPE_CHECKING, Optional, List

from .deformer import Deformer
from .rotation_context import RotationContext
from ..DEF import LIVE2D_FORMAT_VERSION_V2_10_SDK2
from ..param import PivotManager
from ..util import UtMath

if TYPE_CHECKING:
    from .deformer_context import DeformerContext
    from ..model_context import ModelContext
    from ..io import BinaryReader


class RotationDeformer(Deformer):
    # 代替c语言中的指针/引用
    temp1 = [0.0, 0.0]
    temp2 = [0.0, 0.0]
    temp3 = [0.0, 0.0]
    temp4 = [0.0, 0.0]
    temp5 = [0.0, 0.0]
    temp6 = [0.0, 0.0]
    paramOutside = [False]

    def __init__(self):
        super().__init__()
        self.pivotManager: Optional['PivotManager'] = None
        self.affines: Optional[List['AffineEnt']] = None

    def getType(self) -> int:
        return Deformer.TYPE_ROTATION

    def read(self, br: 'BinaryReader'):
        super().read(br)
        self.pivotManager = br.readObject()
        self.affines = br.readObject()
        super().readOpacity(br)

    def init(self, mc) -> 'RotationContext':
        rctx = RotationContext(self)
        rctx.interpolatedAffine = AffineEnt()
        if self.needTransform():
            rctx.transformedAffine = AffineEnt()

        return rctx

    def setupInterpolate(self, modelContext: 'ModelContext', rotationContext: 'RotationContext'):
        if not (self == rotationContext.getDeformer()):
            raise RuntimeError("context not match")

        if not self.pivotManager.checkParamUpdated(modelContext):
            return

        success = RotationDeformer.paramOutside
        success[0] = False
        pivotCount = self.pivotManager.calcPivotValues(modelContext, success)
        rotationContext.setOutsideParam(success[0])
        self.interpolateOpacity(modelContext, self.pivotManager, rotationContext, success)
        pivotIndices = modelContext.getTempPivotTableIndices()
        pivotValues = modelContext.getTempT()
        self.pivotManager.calcPivotIndices(pivotIndices, pivotValues, pivotCount)
        if pivotCount <= 0:
            affine = self.affines[pivotIndices[0]]
            rotationContext.interpolatedAffine.init(affine)
        else:
            if pivotCount == 1:
                affine1 = self.affines[pivotIndices[0]]
                affine2 = self.affines[pivotIndices[1]]
                weight = pivotValues[0]
                rotationContext.interpolatedAffine.originX = affine1.originX + (affine2.originX - affine1.originX) * weight
                rotationContext.interpolatedAffine.originY = affine1.originY + (affine2.originY - affine1.originY) * weight
                rotationContext.interpolatedAffine.scaleX = affine1.scaleX + (affine2.scaleX - affine1.scaleX) * weight
                rotationContext.interpolatedAffine.scaleY = affine1.scaleY + (affine2.scaleY - affine1.scaleY) * weight
                rotationContext.interpolatedAffine.rotationDeg = affine1.rotationDeg + (
                        affine2.rotationDeg - affine1.rotationDeg) * weight
            else:
                if pivotCount == 2:
                    affine1 = self.affines[pivotIndices[0]]
                    affine2 = self.affines[pivotIndices[1]]
                    affine3 = self.affines[pivotIndices[2]]
                    affine4 = self.affines[pivotIndices[3]]
                    weight1 = pivotValues[0]
                    weight2 = pivotValues[1]
                    interpolated1X = affine1.originX + (affine2.originX - affine1.originX) * weight1
                    interpolated2X = affine3.originX + (affine4.originX - affine3.originX) * weight1
                    rotationContext.interpolatedAffine.originX = interpolated1X + (interpolated2X - interpolated1X) * weight2
                    interpolated1Y = affine1.originY + (affine2.originY - affine1.originY) * weight1
                    interpolated2Y = affine3.originY + (affine4.originY - affine3.originY) * weight1
                    rotationContext.interpolatedAffine.originY = interpolated1Y + (interpolated2Y - interpolated1Y) * weight2
                    interpolated1ScaleX = affine1.scaleX + (affine2.scaleX - affine1.scaleX) * weight1
                    interpolated2ScaleX = affine3.scaleX + (affine4.scaleX - affine3.scaleX) * weight1
                    rotationContext.interpolatedAffine.scaleX = interpolated1ScaleX + (interpolated2ScaleX - interpolated1ScaleX) * weight2
                    interpolated1ScaleY = affine1.scaleY + (affine2.scaleY - affine1.scaleY) * weight1
                    interpolated2ScaleY = affine3.scaleY + (affine4.scaleY - affine3.scaleY) * weight1
                    rotationContext.interpolatedAffine.scaleY = interpolated1ScaleY + (interpolated2ScaleY - interpolated1ScaleY) * weight2
                    interpolated1Rotation = affine1.rotationDeg + (affine2.rotationDeg - affine1.rotationDeg) * weight1
                    interpolated2Rotation = affine3.rotationDeg + (affine4.rotationDeg - affine3.rotationDeg) * weight1
                    rotationContext.interpolatedAffine.rotationDeg = interpolated1Rotation + (interpolated2Rotation - interpolated1Rotation) * weight2
                else:
                    if pivotCount == 3:
                        affine1 = self.affines[pivotIndices[0]]
                        affine2 = self.affines[pivotIndices[1]]
                        affine3 = self.affines[pivotIndices[2]]
                        affine4 = self.affines[pivotIndices[3]]
                        affine5 = self.affines[pivotIndices[4]]
                        affine6 = self.affines[pivotIndices[5]]
                        affine7 = self.affines[pivotIndices[6]]
                        affine8 = self.affines[pivotIndices[7]]
                        weight1 = pivotValues[0]
                        weight2 = pivotValues[1]
                        weight3 = pivotValues[2]
                        interpolated1X = affine1.originX + (affine2.originX - affine1.originX) * weight1
                        interpolated2X = affine3.originX + (affine4.originX - affine3.originX) * weight1
                        interpolated3X = affine5.originX + (affine6.originX - affine5.originX) * weight1
                        interpolated4X = affine7.originX + (affine8.originX - affine7.originX) * weight1
                        interpolatedA = (1 - weight3) * (interpolated1X + (interpolated2X - interpolated1X) * weight2) + weight3 * (
                                interpolated3X + (interpolated4X - interpolated3X) * weight2)
                        rotationContext.interpolatedAffine.originX = interpolatedA
                        interpolated1Y = affine1.originY + (affine2.originY - affine1.originY) * weight1
                        interpolated2Y = affine3.originY + (affine4.originY - affine3.originY) * weight1
                        interpolated3Y = affine5.originY + (affine6.originY - affine5.originY) * weight1
                        interpolated4Y = affine7.originY + (affine8.originY - affine7.originY) * weight1
                        interpolatedB = (1 - weight3) * (interpolated1Y + (interpolated2Y - interpolated1Y) * weight2) + weight3 * (
                                interpolated3Y + (interpolated4Y - interpolated3Y) * weight2)
                        rotationContext.interpolatedAffine.originY = interpolatedB
                        interpolated1ScaleX = affine1.scaleX + (affine2.scaleX - affine1.scaleX) * weight1
                        interpolated2ScaleX = affine3.scaleX + (affine4.scaleX - affine3.scaleX) * weight1
                        interpolated3ScaleX = affine5.scaleX + (affine6.scaleX - affine5.scaleX) * weight1
                        interpolated4ScaleX = affine7.scaleX + (affine8.scaleX - affine7.scaleX) * weight1
                        interpolatedC = (1 - weight3) * (interpolated1ScaleX + (interpolated2ScaleX - interpolated1ScaleX) * weight2) + weight3 * (
                                interpolated3ScaleX + (interpolated4ScaleX - interpolated3ScaleX) * weight2)
                        rotationContext.interpolatedAffine.scaleX = interpolatedC
                        interpolated1ScaleY = affine1.scaleY + (affine2.scaleY - affine1.scaleY) * weight1
                        interpolated2ScaleY = affine3.scaleY + (affine4.scaleY - affine3.scaleY) * weight1
                        interpolated3ScaleY = affine5.scaleY + (affine6.scaleY - affine5.scaleY) * weight1
                        interpolated4ScaleY = affine7.scaleY + (affine8.scaleY - affine7.scaleY) * weight1
                        interpolatedD = (1 - weight3) * (interpolated1ScaleY + (interpolated2ScaleY - interpolated1ScaleY) * weight2) + weight3 * (
                                interpolated3ScaleY + (interpolated4ScaleY - interpolated3ScaleY) * weight2)
                        rotationContext.interpolatedAffine.scaleY = interpolatedD
                        interpolated1Rotation = affine1.rotationDeg + (affine2.rotationDeg - affine1.rotationDeg) * weight1
                        interpolated2Rotation = affine3.rotationDeg + (affine4.rotationDeg - affine3.rotationDeg) * weight1
                        interpolated3Rotation = affine5.rotationDeg + (affine6.rotationDeg - affine5.rotationDeg) * weight1
                        interpolated4Rotation = affine7.rotationDeg + (affine8.rotationDeg - affine7.rotationDeg) * weight1
                        interpolatedE = (1 - weight3) * (interpolated1Rotation + (interpolated2Rotation - interpolated1Rotation) * weight2) + weight3 * (
                                interpolated3Rotation + (interpolated4Rotation - interpolated3Rotation) * weight2)
                        rotationContext.interpolatedAffine.rotationDeg = interpolatedE
                    else:
                        if pivotCount == 4:
                            affine1 = self.affines[pivotIndices[0]]
                            affine2 = self.affines[pivotIndices[1]]
                            affine3 = self.affines[pivotIndices[2]]
                            affine4 = self.affines[pivotIndices[3]]
                            affine5 = self.affines[pivotIndices[4]]
                            affine6 = self.affines[pivotIndices[5]]
                            affine7 = self.affines[pivotIndices[6]]
                            affine8 = self.affines[pivotIndices[7]]
                            affine9 = self.affines[pivotIndices[8]]
                            affine10 = self.affines[pivotIndices[9]]
                            affine11 = self.affines[pivotIndices[10]]
                            affine12 = self.affines[pivotIndices[11]]
                            affine13 = self.affines[pivotIndices[12]]
                            affine14 = self.affines[pivotIndices[13]]
                            affine15 = self.affines[pivotIndices[14]]
                            affine16 = self.affines[pivotIndices[15]]
                            weight1 = pivotValues[0]
                            weight2 = pivotValues[1]
                            weight3 = pivotValues[2]
                            weight4 = pivotValues[3]
                            interpolated1X = affine1.originX + (affine2.originX - affine1.originX) * weight1
                            interpolated2X = affine3.originX + (affine4.originX - affine3.originX) * weight1
                            interpolated3X = affine5.originX + (affine6.originX - affine5.originX) * weight1
                            interpolated4X = affine7.originX + (affine8.originX - affine7.originX) * weight1
                            interpolated5X = affine9.originX + (affine10.originX - affine9.originX) * weight1
                            interpolated6X = affine11.originX + (affine12.originX - affine11.originX) * weight1
                            interpolated7X = affine13.originX + (affine14.originX - affine13.originX) * weight1
                            interpolated8X = affine15.originX + (affine16.originX - affine15.originX) * weight1
                            part1 = (1 - weight3) * (interpolated1X + (interpolated2X - interpolated1X) * weight2) + weight3 * (
                                    interpolated3X + (interpolated4X - interpolated3X) * weight2)
                            part2 = (1 - weight3) * (
                                    interpolated5X + (interpolated6X - interpolated5X) * weight2) + weight3 * (
                                            interpolated7X + (interpolated8X - interpolated7X) * weight2)
                            rotationContext.interpolatedAffine.originX = (1 - weight4) * part1 + weight4 * part2
                            interpolated1Y = affine1.originY + (affine2.originY - affine1.originY) * weight1
                            interpolated2Y = affine3.originY + (affine4.originY - affine3.originY) * weight1
                            interpolated3Y = affine5.originY + (affine6.originY - affine5.originY) * weight1
                            interpolated4Y = affine7.originY + (affine8.originY - affine7.originY) * weight1
                            interpolated5Y = affine9.originY + (affine10.originY - affine9.originY) * weight1
                            interpolated6Y = affine11.originY + (affine12.originY - affine11.originY) * weight1
                            interpolated7Y = affine13.originY + (affine14.originY - affine13.originY) * weight1
                            interpolated8Y = affine15.originY + (affine16.originY - affine15.originY) * weight1
                            part3 = (1 - weight3) * (interpolated1Y + (interpolated2Y - interpolated1Y) * weight2) + weight3 * (
                                    interpolated3Y + (interpolated4Y - interpolated3Y) * weight2)
                            part4 = (1 - weight3) * (
                                    interpolated5Y + (interpolated6Y - interpolated5Y) * weight2) + weight3 * (
                                            interpolated7Y + (interpolated8Y - interpolated7Y) * weight2)
                            rotationContext.interpolatedAffine.originY = (1 - weight4) * part3 + weight4 * part4
                            interpolated1ScaleX = affine1.scaleX + (affine2.scaleX - affine1.scaleX) * weight1
                            interpolated2ScaleX = affine3.scaleX + (affine4.scaleX - affine3.scaleX) * weight1
                            interpolated3ScaleX = affine5.scaleX + (affine6.scaleX - affine5.scaleX) * weight1
                            interpolated4ScaleX = affine7.scaleX + (affine8.scaleX - affine7.scaleX) * weight1
                            interpolated5ScaleX = affine9.scaleX + (affine10.scaleX - affine9.scaleX) * weight1
                            interpolated6ScaleX = affine11.scaleX + (affine12.scaleX - affine11.scaleX) * weight1
                            interpolated7ScaleX = affine13.scaleX + (affine14.scaleX - affine13.scaleX) * weight1
                            interpolated8ScaleX = affine15.scaleX + (affine16.scaleX - affine15.scaleX) * weight1
                            part5 = (1 - weight3) * (interpolated1ScaleX + (interpolated2ScaleX - interpolated1ScaleX) * weight2) + weight3 * (
                                    interpolated3ScaleX + (interpolated4ScaleX - interpolated3ScaleX) * weight2)
                            part6 = (1 - weight3) * (
                                    interpolated5ScaleX + (interpolated6ScaleX - interpolated5ScaleX) * weight2) + weight3 * (
                                            interpolated7ScaleX + (interpolated8ScaleX - interpolated7ScaleX) * weight2)
                            rotationContext.interpolatedAffine.scaleX = (1 - weight4) * part5 + weight4 * part6
                            interpolated1ScaleY = affine1.scaleY + (affine2.scaleY - affine1.scaleY) * weight1
                            interpolated2ScaleY = affine3.scaleY + (affine4.scaleY - affine3.scaleY) * weight1
                            interpolated3ScaleY = affine5.scaleY + (affine6.scaleY - affine5.scaleY) * weight1
                            interpolated4ScaleY = affine7.scaleY + (affine8.scaleY - affine7.scaleY) * weight1
                            interpolated5ScaleY = affine9.scaleY + (affine10.scaleY - affine9.scaleY) * weight1
                            interpolated6ScaleY = affine11.scaleY + (affine12.scaleY - affine11.scaleY) * weight1
                            interpolated7ScaleY = affine13.scaleY + (affine14.scaleY - affine13.scaleY) * weight1
                            interpolated8ScaleY = affine15.scaleY + (affine16.scaleY - affine15.scaleY) * weight1
                            part7 = (1 - weight3) * (interpolated1ScaleY + (interpolated2ScaleY - interpolated1ScaleY) * weight2) + weight3 * (
                                    interpolated3ScaleY + (interpolated4ScaleY - interpolated3ScaleY) * weight2)
                            part8 = (1 - weight3) * (
                                    interpolated5ScaleY + (interpolated6ScaleY - interpolated5ScaleY) * weight2) + weight3 * (
                                            interpolated7ScaleY + (interpolated8ScaleY - interpolated7ScaleY) * weight2)
                            rotationContext.interpolatedAffine.scaleY = (1 - weight4) * part7 + weight4 * part8
                            interpolated1Rotation = affine1.rotationDeg + (affine2.rotationDeg - affine1.rotationDeg) * weight1
                            interpolated2Rotation = affine3.rotationDeg + (affine4.rotationDeg - affine3.rotationDeg) * weight1
                            interpolated3Rotation = affine5.rotationDeg + (affine6.rotationDeg - affine5.rotationDeg) * weight1
                            interpolated4Rotation = affine7.rotationDeg + (affine8.rotationDeg - affine7.rotationDeg) * weight1
                            interpolated5Rotation = affine9.rotationDeg + (affine10.rotationDeg - affine9.rotationDeg) * weight1
                            interpolated6Rotation = affine11.rotationDeg + (affine12.rotationDeg - affine11.rotationDeg) * weight1
                            interpolated7Rotation = affine13.rotationDeg + (affine14.rotationDeg - affine13.rotationDeg) * weight1
                            interpolated8Rotation = affine15.rotationDeg + (affine16.rotationDeg - affine15.rotationDeg) * weight1
                            part9 = (1 - weight3) * (interpolated1Rotation + (interpolated2Rotation - interpolated1Rotation) * weight2) + weight3 * (
                                    interpolated3Rotation + (interpolated4Rotation - interpolated3Rotation) * weight2)
                            part10 = (1 - weight3) * (
                                    interpolated5Rotation + (interpolated6Rotation - interpolated5Rotation) * weight2) + weight3 * (
                                            interpolated7Rotation + (interpolated8Rotation - interpolated7Rotation) * weight2)
                            rotationContext.interpolatedAffine.rotationDeg = (1 - weight4) * part9 + weight4 * part10
                        else:
                            affineCount = int(pow(2, pivotCount))
                            weights = [0.0] * (affineCount)
                            for i in range(0, affineCount, 1):
                                index = i
                                weight = 1
                                for j in range(0, pivotCount, 1):
                                    weight *= (1 - pivotValues[j]) if (index % 2 == 0) else pivotValues[j]
                                    index //= 2

                                weights[i] = weight

                            affines = []
                            for i in range(0, affineCount, 1):
                                affines[i] = self.affines[pivotIndices[i]]

                            originX = 0
                            originY = 0
                            scaleX = 0
                            scaleY = 0
                            rotationDeg = 0
                            for i in range(0, affineCount, 1):
                                originX += weights[i] * affines[i].originX
                                originY += weights[i] * affines[i].originY
                                scaleX += weights[i] * affines[i].scaleX
                                scaleY += weights[i] * affines[i].scaleY
                                rotationDeg += weights[i] * affines[i].rotationDeg

                            rotationContext.interpolatedAffine.originX = originX
                            rotationContext.interpolatedAffine.originY = originY
                            rotationContext.interpolatedAffine.scaleX = scaleX
                            rotationContext.interpolatedAffine.scaleY = scaleY
                            rotationContext.interpolatedAffine.rotationDeg = rotationDeg

        affine = self.affines[pivotIndices[0]]
        rotationContext.interpolatedAffine.reflectX = affine.reflectX
        rotationContext.interpolatedAffine.reflectY = affine.reflectY

    def setupTransform(self, modelContext: 'ModelContext', rotationContext: 'RotationContext'):
        if not (self == rotationContext.getDeformer()):
            raise RuntimeError("Invalid Deformer")

        rotationContext.setAvailable(True)
        if not self.needTransform():
            rotationContext.setTotalScale_notForClient(rotationContext.interpolatedAffine.scaleX)
            rotationContext.setTotalOpacity(rotationContext.getInterpolatedOpacity())
        else:
            targetId = self.getTargetId()
            if rotationContext.tmpDeformerIndex == Deformer.DEFORMER_INDEX_NOT_INIT:
                rotationContext.tmpDeformerIndex = modelContext.getDeformerIndex(targetId)

            if rotationContext.tmpDeformerIndex < 0:
                print("deformer is not reachable")
                rotationContext.setAvailable(False)
            else:
                deformer = modelContext.getDeformer(rotationContext.tmpDeformerIndex)
                if deformer is not None:
                    deformerContext = modelContext.getDeformerContext(rotationContext.tmpDeformerIndex)
                    originPoint = RotationDeformer.temp1
                    originPoint[0] = rotationContext.interpolatedAffine.originX
                    originPoint[1] = rotationContext.interpolatedAffine.originY
                    directionVector = RotationDeformer.temp2
                    directionVector[0] = 0
                    directionVector[1] = -0.1
                    deformerType = deformerContext.getDeformer().getType()
                    if deformerType == Deformer.TYPE_ROTATION:
                        directionVector[1] = -10
                    else:
                        directionVector[1] = -0.1

                    transformedDirection = RotationDeformer.temp3
                    self.getDirectionOnDst(modelContext, deformer, deformerContext, originPoint, directionVector, transformedDirection)
                    angle = UtMath.getAngleNotAbs(directionVector, transformedDirection)
                    deformer.transformPoints(modelContext, deformerContext, originPoint, originPoint, 1, 0, 2)
                    rotationContext.transformedAffine.originX = originPoint[0]
                    rotationContext.transformedAffine.originY = originPoint[1]
                    rotationContext.transformedAffine.scaleX = rotationContext.interpolatedAffine.scaleX
                    rotationContext.transformedAffine.scaleY = rotationContext.interpolatedAffine.scaleY
                    rotationContext.transformedAffine.rotationDeg = rotationContext.interpolatedAffine.rotationDeg - angle * UtMath.RAD_TO_DEG
                    totalScale = deformerContext.getTotalScale()
                    rotationContext.setTotalScale_notForClient(totalScale * rotationContext.transformedAffine.scaleX)
                    totalOpacity = deformerContext.getTotalOpacity()
                    rotationContext.setTotalOpacity(totalOpacity * rotationContext.getInterpolatedOpacity())
                    rotationContext.transformedAffine.reflectX = rotationContext.interpolatedAffine.reflectX
                    rotationContext.transformedAffine.reflectY = rotationContext.interpolatedAffine.reflectY
                    rotationContext.setAvailable(deformerContext.isAvailable())
                else:
                    rotationContext.setAvailable(False)

    def transformPoints(self, modelContext: 'ModelContext', rotationContext: 'RotationContext', srcPoints: List[float], dstPoints: List[float],
                        numPoint: int, ptOffset: int, ptStep: int):
        if not (self == rotationContext.getDeformer()):
            raise RuntimeError("context not match")

        affine = rotationContext.transformedAffine if rotationContext.transformedAffine is not None else rotationContext.interpolatedAffine
        sinRotation = math.sin(UtMath.DEG_TO_RAD * affine.rotationDeg)
        cosRotation = math.cos(UtMath.DEG_TO_RAD * affine.rotationDeg)
        totalScale = rotationContext.getTotalScale()
        reflectX = -1 if affine.reflectX else 1
        reflectY = -1 if affine.reflectY else 1
        scaleX = cosRotation * totalScale * reflectX
        shearX = -sinRotation * totalScale * reflectY
        shearY = sinRotation * totalScale * reflectX
        scaleY = cosRotation * totalScale * reflectY
        originX = affine.originX
        originY = affine.originY
        totalPoints = numPoint * ptStep
        for i in range(ptOffset, totalPoints, ptStep):
            x = srcPoints[i]
            y = srcPoints[i + 1]
            dstPoints[i] = scaleX * x + shearX * y + originX
            dstPoints[i + 1] = shearY * x + scaleY * y + originY

    @staticmethod
    def getDirectionOnDst(modelContext: 'ModelContext', targetDeformer: 'Deformer', targetDeformerContext: 'DeformerContext', srcOrigin, srcDir, retDir):
        if not (targetDeformer == targetDeformerContext.getDeformer()):
            raise RuntimeError("context not match")

        transformedOrigin = RotationDeformer.temp4
        transformedOrigin[0] = srcOrigin[0]
        transformedOrigin[1] = srcOrigin[1]
        targetDeformer.transformPoints(modelContext, targetDeformerContext, transformedOrigin, transformedOrigin, 1, 0, 2)
        transformedPoint = RotationDeformer.temp5
        testPoint = RotationDeformer.temp6
        maxIterations = 10
        stepSize = 1
        for i in range(0, maxIterations, 1):
            testPoint[0] = srcOrigin[0] + stepSize * srcDir[0]
            testPoint[1] = srcOrigin[1] + stepSize * srcDir[1]
            targetDeformer.transformPoints(modelContext, targetDeformerContext, testPoint, transformedPoint, 1, 0, 2)
            transformedPoint[0] -= transformedOrigin[0]
            transformedPoint[1] -= transformedOrigin[1]
            if transformedPoint[0] != 0 or transformedPoint[1] != 0:
                retDir[0] = transformedPoint[0]
                retDir[1] = transformedPoint[1]
                return

            testPoint[0] = srcOrigin[0] - stepSize * srcDir[0]
            testPoint[1] = srcOrigin[1] - stepSize * srcDir[1]
            targetDeformer.transformPoints(modelContext, targetDeformerContext, testPoint, transformedPoint, 1, 0, 2)
            transformedPoint[0] -= transformedOrigin[0]
            transformedPoint[1] -= transformedOrigin[1]
            if transformedPoint[0] != 0 or transformedPoint[1] != 0:
                transformedPoint[0] = -transformedPoint[0]
                transformedPoint[1] = -transformedPoint[1]
                retDir[0] = transformedPoint[0]
                retDir[1] = transformedPoint[1]
                return

            stepSize *= 0.1

        print("Invalid state\n")


class AffineEnt:

    def __init__(self):
        self.originX = 0
        self.originY = 0
        self.scaleX = 1
        self.scaleY = 1
        self.rotationDeg = 0
        self.reflectX = False
        self.reflectY = False

    def init(self, other):
        self.originX = other.originX
        self.originY = other.originY
        self.scaleX = other.scaleX
        self.scaleY = other.scaleY
        self.rotationDeg = other.rotationDeg
        self.reflectX = other.reflectX
        self.reflectY = other.reflectY

    def read(self, br: 'BinaryReader'):
        self.originX = br.readFloat32()
        self.originY = br.readFloat32()
        self.scaleX = br.readFloat32()
        self.scaleY = br.readFloat32()
        self.rotationDeg = br.readFloat32()
        if br.getFormatVersion() >= LIVE2D_FORMAT_VERSION_V2_10_SDK2:
            self.reflectX = br.readBoolean()
            self.reflectY = br.readBoolean()
