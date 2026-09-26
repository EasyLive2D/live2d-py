from typing import TYPE_CHECKING

from .idraw_data import IDrawData
from .mesh_context import MeshContext
from ..DEF import LIVE2D_FORMAT_VERSION_V2_8_TEX_OPTION, VERTEX_STEP, VERTEX_TYPE, VERTEX_OFFSET, \
    VERTEX_TYPE_OFFSET0_STEP2, REVERSE_TEXTURE_T, VERTEX_TYPE_OFFSET2_STEP5
from ..param import PivotManager
from ..util import UtInterpolate

if TYPE_CHECKING:
    from ..model_context import ModelContext


class Mesh(IDrawData):
    INSTANCE_COUNT = 0
    MASK_COLOR_COMPOSITION = 30
    COLOR_COMPOSITION_NORMAL = 0
    COLOR_COMPOSITION_SCREEN = 1
    COLOR_COMPOSITION_MULTIPLY = 2
    paramOutside = [False]

    def __init__(self):
        super().__init__()
        self.textureNo = -1
        self.pointCount = 0
        self.polygonCount = 0
        self.optionFlag = None
        self.indexArray = None
        self.pivotPoints = None
        self.uvs = None
        self.colorCompositionType = Mesh.COLOR_COMPOSITION_NORMAL
        self.culling = True
        self.instanceNo = Mesh.INSTANCE_COUNT
        Mesh.INSTANCE_COUNT += 1

    def setTextureNo(self, texture_no):
        self.textureNo = texture_no

    def getTextureNo(self):
        return self.textureNo

    def getUvs(self):
        return self.uvs

    def getOptionFlag(self):
        return self.optionFlag

    def getNumPoints(self):
        return self.pointCount

    def getType(self):
        return IDrawData.TYPE_MESH

    def read(self, br):
        super().read(br)
        self.textureNo = br.readInt32()
        self.pointCount = br.readInt32()
        self.polygonCount = br.readInt32()
        obj = br.readObject()
        self.indexArray = [0] * (self.polygonCount * 3)
        for i in range(self.polygonCount * 3 - 1, 0 - 1, -1):
            self.indexArray[i] = obj[i]

        self.pivotPoints = br.readObject()
        self.uvs = br.readObject()
        if br.getFormatVersion() >= LIVE2D_FORMAT_VERSION_V2_8_TEX_OPTION:
            self.optionFlag = br.readInt32()
            if self.optionFlag != 0:
                if (self.optionFlag & 1) != 0:
                    _ = br.readInt32()
                    raise RuntimeError("not handled")

                if (self.optionFlag & Mesh.MASK_COLOR_COMPOSITION) != 0:
                    self.colorCompositionType = (self.optionFlag & Mesh.MASK_COLOR_COMPOSITION) >> 1
                else:
                    self.colorCompositionType = Mesh.COLOR_COMPOSITION_NORMAL

                if (self.optionFlag & 32) != 0:
                    self.culling = False
        else:
            self.optionFlag = 0

    def init(self, modelContext):
        ctx = MeshContext(self)
        vertexCount = self.pointCount * VERTEX_STEP
        needTransform = self.needTransform()
        if ctx.interpolatedPoints is not None:
            ctx.interpolatedPoints = None

        ctx.interpolatedPoints = [0.0] * (vertexCount)
        if ctx.transformedPoints is not None:
            ctx.transformedPoints = None

        ctx.transformedPoints = [0.0] * (vertexCount) if needTransform else None
        vertexType = VERTEX_TYPE

        if vertexType == VERTEX_TYPE_OFFSET0_STEP2:
            if REVERSE_TEXTURE_T:
                for i in range(self.pointCount - 1, 0 - 1, -1):
                    uvOffset = i << 1
                    self.uvs[uvOffset + 1] = 1 - self.uvs[uvOffset + 1]
        elif vertexType == VERTEX_TYPE_OFFSET2_STEP5:
            for i in range(self.pointCount - 1, 0 - 1, -1):
                uvOffset = i << 1
                vertexOffset = i * VERTEX_STEP
                uvX = self.uvs[uvOffset]
                uvY = self.uvs[uvOffset + 1]
                ctx.interpolatedPoints[vertexOffset] = uvX
                ctx.interpolatedPoints[vertexOffset + 1] = uvY
                ctx.interpolatedPoints[vertexOffset + 4] = 0
                if needTransform:
                    ctx.transformedPoints[vertexOffset] = uvX
                    ctx.transformedPoints[vertexOffset + 1] = uvY
                    ctx.transformedPoints[vertexOffset + 4] = 0

        return ctx

    def setupInterpolate(self, modelContext, meshContext):
        ctx = meshContext
        if not (self == ctx.getDrawData()):
            print("### assert!! ### ")

        if not self.pivotMgr.checkParamUpdated(modelContext):
            return

        super().setupInterpolate(modelContext, ctx)
        if ctx.paramOutside[0]:
            return

        paramOutside = Mesh.paramOutside
        paramOutside[0] = False
        UtInterpolate.interpolatePoints(modelContext, self.pivotMgr, paramOutside, self.pointCount, self.pivotPoints, ctx.interpolatedPoints,
                                        VERTEX_OFFSET, VERTEX_STEP)

    def setupTransform(self, modelContext, drawContext=None):
        if not (self == drawContext.getDrawData()):
            raise RuntimeError("context not match")

        paramOutside = False
        if drawContext.paramOutside[0]:
            paramOutside = True

        if not paramOutside:
            super().setupTransform(modelContext)
            if self.needTransform():
                target_id = self.getTargetId()
                if drawContext.tmpDeformerIndex == IDrawData.DEFORMER_INDEX_NOT_INIT:
                    drawContext.tmpDeformerIndex = modelContext.getDeformerIndex(target_id)

                if drawContext.tmpDeformerIndex < 0:
                    print(f"deformer not found: {target_id}")
                else:
                    deformer = modelContext.getDeformer(drawContext.tmpDeformerIndex)
                    deformerContext = modelContext.getDeformerContext(drawContext.tmpDeformerIndex)
                    if deformer is not None and not deformerContext.isOutsideParam():
                        deformer.transformPoints(modelContext, deformerContext, drawContext.interpolatedPoints, drawContext.transformedPoints, self.pointCount,
                                          VERTEX_OFFSET, VERTEX_STEP)
                        drawContext.available = True
                    else:
                        drawContext.available = False

                    drawContext.baseOpacity = deformerContext.getTotalOpacity()

    def draw(self, dp, mctx: 'ModelContext', dctx: 'MeshContext'):
        if not (self == dctx.getDrawData()):
            raise RuntimeError("context not match")

        if dctx.paramOutside[0]:
            return

        texNr = self.textureNo
        if texNr < 0:
            texNr = 1

        opacity = (self.getOpacity(dctx) *
                    dctx.partsOpacity *
                    dctx.baseOpacity)
        # print("op1: ", opacity)
        vertices = dctx.transformedPoints if (dctx.transformedPoints is not None) else dctx.interpolatedPoints
        dp.setClipBufPre_clipContextForDraw(dctx.clipBufPre_clipContext)
        dp.setCulling(self.culling)
        pctx = mctx.getPartsContext(dctx.partsIndex)
        dp.drawTexture(texNr, pctx.screenColor, self.indexArray, vertices, self.uvs, opacity, self.colorCompositionType,
                       pctx.multiplyColor)

    def getIndexArray(self):
        return self.indexArray
