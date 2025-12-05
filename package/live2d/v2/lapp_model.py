import math
import os.path
from random import random, choice
from typing import TYPE_CHECKING, List, Union, Tuple

from .core import UtSystem, log
from .framework import L2DBaseModel, L2DTargetPoint, L2DEyeBlink
from .lapp_define import MotionPriority
from .matrix_manager import MatrixManager
from .model_setting_json import ModelSettingJson
from .params import Parameter

if TYPE_CHECKING:
    from .core.draw import MeshContext, Mesh


class LAppModel(L2DBaseModel):

    def __init__(self):
        super().__init__()
        self.modelHomeDir = ""
        self.modelSetting = None
        self.matrixManager = MatrixManager()
        self.dragMgr = L2DTargetPoint()
        self.dragMgr.setPoint(0.0, 0.0)
        self.autoBreath = True
        self.autoBlink = True

        self.finishCallback = None

        self.__clearFlag = False

    def LoadModelJson(self, modelSettingPath: str, version: str = "#version 120\n", disable_precision: bool = False):
        self.setUpdating(True)
        self.setInitialized(False)
        self.modelHomeDir = os.path.dirname(modelSettingPath) + "/"
        self.modelSetting = ModelSettingJson()

        self.modelSetting.loadModelSetting(modelSettingPath)

        path = self.modelHomeDir + self.modelSetting.getModelFile()

        self.loadModelData(path, version, disable_precision)

        for i in range(self.modelSetting.getTextureNum()):
            tex_paths = self.modelHomeDir + self.modelSetting.getTextureFile(i)
            self.loadTexture(i, tex_paths)

        if self.modelSetting.getExpressionNum() > 0:
            self.expressions = {}
            for j in range(self.modelSetting.getExpressionNum()):
                exp_name = self.modelSetting.getExpressionName(j)
                exp_file_path = self.modelHomeDir + self.modelSetting.getExpressionFile(j)
                self.loadExpression(exp_name, exp_file_path)
        else:
            self.expressionManager = None
            self.expressions = {}

        if self.eyeBlink is None:
            self.eyeBlink = L2DEyeBlink()

        if self.modelSetting.getPhysicsFile() is not None:
            self.loadPhysics(self.modelHomeDir + self.modelSetting.getPhysicsFile())
        else:
            self.physics = None

        if self.modelSetting.getPoseFile() is not None:
            pose = self.loadPose(self.modelHomeDir + self.modelSetting.getPoseFile())
            pose.updateParam(self.live2DModel)

        if self.modelSetting.getLayout() is not None:
            layout = self.modelSetting.getLayout()
            if layout.get("width", None) is not None:
                self.modelMatrix.setWidth(layout["width"])
            if layout.get("height", None) is not None:
                self.modelMatrix.setHeight(layout["height"])
            if layout.get("x", None) is not None:
                self.modelMatrix.setX(layout["x"])
            if layout.get("y", None) is not None:
                self.modelMatrix.setY(layout["y"])
            if layout.get("center_x", None) is not None:
                self.modelMatrix.centerX(layout["center_x"])
            if layout.get("center_y", None) is not None:
                self.modelMatrix.centerY(layout["center_y"])
            if layout.get("top", None) is not None:
                self.modelMatrix.top(layout["top"])
            if layout.get("bottom", None) is not None:
                self.modelMatrix.bottom(layout["bottom"])
            if layout.get("left", None) is not None:
                self.modelMatrix.left(layout["left"])
            if layout.get("right", None) is not None:
                self.modelMatrix.right(layout["right"])

        for j in range(self.modelSetting.getInitParamNum()):
            self.live2DModel.setParamFloat(self.modelSetting.getInitParamID(j),
                                           self.modelSetting.getInitParamValue(j))

        for j in range(self.modelSetting.getInitPartsVisibleNum()):
            self.live2DModel.setPartsOpacity(self.modelSetting.getInitPartsVisibleID(j),
                                             self.modelSetting.getInitPartsVisibleValue(j))

        self.live2DModel.saveParam()
        self.__preloadMotionGroup(MotionPriority.IDLE)
        self.mainMotionManager.stopAllMotions()
        self.setUpdating(False)
        self.setInitialized(True)

    def Resize(self, ww: int, wh: int):
        self.matrixManager.onResize(ww, wh)
        self.live2DModel.resize(ww, wh)

    def Drag(self, x: float, y: float):
        scx, scy = self.matrixManager.screenToScene(x, y)
        self.dragMgr.setPoint(scx, scy)

    def IsMotionFinished(self) -> bool:
        return self.mainMotionManager.isFinished()

    def SetOffset(self, dx: float, dy: float):
        self.matrixManager.setOffset(dx, dy)

    def SetScale(self, scale: float):
        self.matrixManager.setScale(scale)

    def SetParameterValue(self, paramId: str, value: float, weight: float = 1.0):
        self.live2DModel.setParamFloat(paramId, value, weight)
        mdc = self.live2DModel.modelContext
        idx = mdc.getParamIndex(paramId)
        if idx < len(mdc.savedParamValues):
            mdc.savedParamValues[idx] = mdc.savedParamValues[idx] * (1 - weight) + value * weight


    def AddParameterValue(self, paramId: str, value: float, weight: float = 1.0):
        self.live2DModel.addToParamFloat(paramId, value, weight)
        mdc = self.live2DModel.modelContext
        idx = mdc.getParamIndex(paramId)
        if idx < len(mdc.savedParamValues):
            mdc.savedParamValues[idx] = mdc.savedParamValues[idx] * (1 - weight) + value * weight

    def SetAutoBreathEnable(self, enable: bool):
        self.autoBreath = enable

    def SetAutoBlinkEnable(self, enable: bool):
        self.autoBlink = enable

    def GetParameterCount(self) -> int:
        return len(self.live2DModel.getModelContext().paramIdList)

    def GetParameter(self, index: int) -> Parameter:
        p = Parameter()
        p.value = self.live2DModel.getParamFloat(index)
        p.max = self.live2DModel.getModelContext().getParamMax(index)
        p.min = self.live2DModel.getModelContext().getParamMin(index)
        inner_params = self.live2DModel.getModelImpl().paramDefSet.getParamDefFloatList()
        is_inner = index < len(inner_params)
        p.type = Parameter.TYPE_INNER if is_inner else Parameter.TYPE_OUTER
        p.default = inner_params[index].defaultValue if is_inner else 0
        p.id = self.live2DModel.getModelContext().paramIdList[index]
        return p

    def GetPartCount(self) -> int:
        return len(self.live2DModel.getModelImpl().getPartsDataList())

    def GetPartId(self, index: int) -> str:
        return self.live2DModel.getModelContext().getPartsContext(index).partsData.id.id

    def GetPartIds(self) -> List[str]:
        return [str(i.id) for i in self.live2DModel.getModelContext().partsDataList]

    def SetPartOpacity(self, index: int, opacity: float):
        self.live2DModel.setPartsOpacity(index, opacity)

    def Update(self):
        self.dragMgr.update()
        self.setDrag(self.dragMgr.getX(), self.dragMgr.getY())

        time_m_sec = UtSystem.getUserTimeMSec() - self.startTimeMSec
        time_sec = time_m_sec / 1000.0
        t = time_sec * 2 * math.pi
        if self.mainMotionManager.isFinished():
            if callable(self.finishCallback):
                self.finishCallback()
                self.finishCallback = None

        updated = False
        if self.__clearFlag:
            self.mainMotionManager.stopAllMotions()
            if self.pose:
                self.pose.initParam(self.live2DModel)

            self.__clearFlag = False
        else:
            self.live2DModel.loadParam()
            updated = self.mainMotionManager.updateParam(self.live2DModel)
        self.live2DModel.saveParam()

        if not updated:
            if self.autoBlink and self.eyeBlink is not None:
                self.eyeBlink.updateParam(self.live2DModel)

        if self.expressionManager is not None and self.expressions is not None and not self.expressionManager.isFinished():
            self.expressionManager.updateParam(self.live2DModel)

        self.live2DModel.addToParamFloat("PARAM_ANGLE_X", self.dragX * 30, 1)
        self.live2DModel.addToParamFloat("PARAM_ANGLE_Y", self.dragY * 30, 1)
        self.live2DModel.addToParamFloat("PARAM_ANGLE_Z", (self.dragX * self.dragY) * -30, 1)
        self.live2DModel.addToParamFloat("PARAM_BODY_ANGLE_X", self.dragX * 10, 1)
        self.live2DModel.addToParamFloat("PARAM_EYE_BALL_X", self.dragX, 1)
        self.live2DModel.addToParamFloat("PARAM_EYE_BALL_Y", self.dragY, 1)

        if self.autoBreath:
            self.live2DModel.addToParamFloat("PARAM_ANGLE_X", float((15 * math.sin(t / 6.5345))), 0.5)
            self.live2DModel.addToParamFloat("PARAM_ANGLE_Y", float((8 * math.sin(t / 3.5345))), 0.5)
            self.live2DModel.addToParamFloat("PARAM_ANGLE_Z", float((10 * math.sin(t / 5.5345))), 0.5)
            self.live2DModel.addToParamFloat("PARAM_BODY_ANGLE_X", float((4 * math.sin(t / 15.5345))), 0.5)
            self.live2DModel.setParamFloat("PARAM_BREATH", float((0.5 + 0.5 * math.sin(t / 3.2345))), 1)

        if self.physics is not None:
            self.physics.updateParam(self.live2DModel)

        if self.pose is not None:
            self.pose.updateParam(self.live2DModel)

    def SetRandomExpression(self):
        tmp = []
        for name in self.expressions:
            tmp.append(name)

        if len(tmp) == 0:
            return
        no = int(random() * len(tmp))
        self.SetExpression(tmp[no])

    def StartRandomMotion(self, name=None, priority=MotionPriority.IDLE, onStartMotionHandler=None,
                          onFinishMotionHandler=None):
        if name is None:
            names = self.modelSetting.getMotionNames()
            if names is not None:
                name = choice(names)
            else:
                name = MotionPriority.IDLE
        count = self.modelSetting.getMotionNum(name)
        no = int(random() * count)
        self.StartMotion(name, no, priority, onStartMotionHandler, onFinishMotionHandler)

    def StartMotion(self, name, no, priority, onStartMotionHandler=None, onFinishMotionHandler=None):
        motion_name = self.modelSetting.getMotionFile(name, no)
        if motion_name is None or motion_name == "":
            if callable(onStartMotionHandler):
                onStartMotionHandler(name, no)
            if callable(onFinishMotionHandler):
                onFinishMotionHandler()
            return

        if priority == MotionPriority.FORCE:
            self.mainMotionManager.setReservePriority(priority)
        elif not self.mainMotionManager.reserveMotion(priority):
            return

        if self.motions.get(name) is None:
            mtn = self.loadMotion(None, self.modelHomeDir + motion_name)
        else:
            mtn = self.motions[name]

        self.finishCallback = onFinishMotionHandler
        if callable(onStartMotionHandler):
            onStartMotionHandler(name, no)
        self.__setFadeInFadeOut(name, no, priority, mtn)

    def SetExpression(self, name: str):
        motion = self.expressions[name]
        self.expressionManager.startMotion(motion, False)

    def Draw(self):
        # 根据设置的参数更新绘图所需参数
        self.live2DModel.update()
        model_matrix = self.modelMatrix
        tmp_matrix = self.matrixManager.getMvp(model_matrix)
        self.live2DModel.setMatrix(tmp_matrix)
        self.live2DModel.draw()

    def HitTest(self, hitAreaName: str, testX, testY) -> Union[str, None]:
        size = self.modelSetting.getHitAreaNum()
        for i in range(size):
            area_id = self.modelSetting.getHitAreaName(i)
            draw_id = self.modelSetting.getHitAreaID(i)
            if self.hitTestSimple(draw_id, testX, testY):
                return area_id

        return None

    def __preloadMotionGroup(self, name):
        for i in range(self.modelSetting.getMotionNum(name)):
            file = self.modelSetting.getMotionFile(name, i)
            motion = self.loadMotion(file, self.modelHomeDir + file)

            motion.setFadeIn(self.modelSetting.getMotionFadeIn(name, i))
            motion.setFadeOut(self.modelSetting.getMotionFadeOut(name, i))

    def __setFadeInFadeOut(self, name, no, priority, motion):
        motion.setFadeIn(self.modelSetting.getMotionFadeIn(name, no))
        motion.setFadeOut(self.modelSetting.getMotionFadeOut(name, no))

        self.mainMotionManager.startMotionPrio(motion, priority)

    def HitPart(self, src_x: float, src_y: float, topOnly: bool = False) -> List[str]:
        src_x, src_y = self.matrixManager.screenToScene(src_x, src_y)
        mx, my = self.matrixManager.invertTransform(src_x, src_y)
        mctx = self.live2DModel.getModelContext()
        draw_orders = mctx.nextList_drawIndex
        if not draw_orders:
            return []
        draw_orders = reversed(draw_orders)
        hit_part_ids = []
        for idx in draw_orders:
            if idx == -1:
                continue

            ddcxt: 'MeshContext' = mctx.getDrawContext(idx)
            pctx = mctx.getPartsContext(ddcxt.partsIndex)
            parent_part = pctx.partsData
            if not parent_part.visible or pctx.partsOpacity < 0.1:
                continue

            part_id = str(parent_part.id)
            if part_id in hit_part_ids:
                continue

            dd: 'Mesh' = ddcxt.drawData
            vertices = ddcxt.getTransformedPoints()
            indices = dd.indexArray
            size = len(indices)
            for i in range(0, size, 3):
                p1_idx = indices[i] * 2
                p2_idx = indices[i + 1] * 2
                p3_idx = indices[i + 2] * 2
                if not self.__isInTriangle(mx, my,
                                           vertices[p1_idx], vertices[p1_idx + 1],
                                           vertices[p2_idx], vertices[p2_idx + 1],
                                           vertices[p3_idx], vertices[p3_idx + 1]):
                    continue

                hit_part_ids.append(part_id)
                if topOnly:
                    return hit_part_ids
                break

        return hit_part_ids

    @staticmethod
    def __isInTriangle(x, y, x0, y0, x1, y1, x2, y2) -> bool:
        if x < min(x0, x1, x2):
            return False
        if x > max(x0, x1, x2):
            return False
        if y < min(y0, y1, y2):
            return False
        if y > max(y0, y1, y2):
            return False

        d_x = x - x2
        d_y = y - y2
        d_x21 = x2 - x1
        d_y12 = y1 - y2
        D = d_y12 * (x0 - x2) + d_x21 * (y0 - y2)
        s = d_y12 * d_x + d_x21 * d_y
        t = (y2 - y0) * d_x + (x0 - x2) * d_y
        if D < 0:
            return s <= 0 and t <= 0 and s + t >= D
        return s >= 0 and t >= 0 and s + t <= D

    def setPartScreenColor(self, part_index: int, r: float, g: float, b: float, a: float):
        self.live2DModel.modelContext.setPartScreenColor(part_index, r, g, b, a)

    def GetPartScreenColor(self, part_index: int) -> List[float]:
        """
        any modification to the returned list is equivalent to setPartScreenColor
        :param part_index:
        :return: ref of the part screen color
        """
        return self.live2DModel.modelContext.getPartScreenColor(part_index)

    def SetPartMultiplyColor(self, part_index: int, r: float, g: float, b: float, a: float):
        self.live2DModel.modelContext.setPartMultiplyColor(part_index, r, g, b, a)

    def GetPartMultiplyColor(self, part_index: int) -> List[float]:
        """
        modify the returned list is equivalent to setPartMultiplyColor
        :param part_index:
        :return: ref of the part multiply color
        """
        return self.live2DModel.modelContext.getPartMultiplyColor(part_index)

    def ClearMotions(self):
        self.__clearFlag = True
    
    def ResetExpression(self):
        self.expressionManager.stopAllMotions()

    def Rotate(self, deg: float):
        self.matrixManager.rotate(deg)

    def GetCanvasSize(self) -> Tuple[int, int]:
        impl = self.live2DModel.getModelImpl()
        return impl.getCanvasWidth(), impl.getCanvasHeight()

    def GetCanvasSizePixel(self) -> Tuple[int, int]:
        impl = self.live2DModel.getModelImpl()
        return impl.getCanvasWidth(), impl.getCanvasHeight()

    def GetPixelsPerUnit(self) -> int:
        return 1