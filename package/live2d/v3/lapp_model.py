"""LAppModel — Python wrapper around C++ v3.Model."""
import time
from typing import Callable, Optional

from ._v3cpp import Model
from .params import Parameter


class LAppModel:
    """High-level Live2D model (pure Python implementation on top of C++ Model)."""

    def __init__(self):
        self._model = Model()
        self._renderer_created = False
        self._lastExpression = ""       # default expression to restore after fadeout
        self._fadeout = -1              # fadeout duration (ms), -1 = none
        self._expStartedAt = -1         # timestamp when temporary expression started
        self._motions_cache = None      # cached result of GetMotions()
        self._sound_cache = {}          # (group, index) -> sound path
        self._lastFrame = time.time()   # for delta time calculation
        self._offset_x, self._offset_y = 0.0, 0.0  # tracked for SetOffsetX/SetOffsetY

    # ---- loading / resize / draw ----

    def LoadModelJson(self, modelJsonPath: str, maskBufferCount: int = 2):
        self._model.LoadModelJson(modelJsonPath)
        if not self._renderer_created:
            self._model.CreateRenderer(maskBufferCount)
            self._renderer_created = True

    def Resize(self, width: int, height: int):
        self._model.Resize(width, height)

    def Draw(self):
        self._model.Draw()

    def Update(self):
        now = time.time()
        dt = min(now - self._lastFrame, 0.1)  # cap to prevent large jumps
        self._lastFrame = now
        self._model.Update(dt)
        # motionUpdated = False
        # self._model.LoadParameters()
        # if not self._model.IsMotionFinished():
        #     motionUpdated = self._model.UpdateMotion(dt)
        # self._model.SaveParameters()
        # if not motionUpdated:
        #     self._model.UpdateBlink(dt)

        # self._model.UpdateExpression(dt)
        # self._model.UpdateDrag(dt)
        # self._model.UpdateBreath(dt)
        # self._model.UpdatePhysics(dt)
        # self._model.UpdatePose(dt)

    # ---- motion ----

    def StartMotion(self, group: str, no: int, priority: int = 3,
                    onStartMotionHandler=None, onFinishMotionHandler=None):
        self._model.StartMotion(
            group, no, priority,
            onStart=onStartMotionHandler, onFinish=onFinishMotionHandler)

    def StartRandomMotion(self, group: Optional[str] = None, priority: int = 3,
                          onStartMotionHandler=None, onFinishMotionHandler=None):
        self._model.StartRandomMotion(
            group, priority,
            onStart=onStartMotionHandler, onFinish=onFinishMotionHandler)

    def IsMotionFinished(self) -> bool:
        return self._model.IsMotionFinished()

    def StopAllMotions(self):
        self._model.StopAllMotions()

    def LoadExtraMotion(self, group: str, motionJsonPath: str) -> int:
        return self._model.LoadExtraMotion(group, motionJsonPath)

    def GetMotionGroups(self) -> dict:
        """Return {group_name: motion_count} dict."""
        motions = self.GetMotions()
        result = {}
        for group, entries in motions.items():
            result[group] = len(entries)
        return result

    def GetMotions(self) -> dict:
        """Return {group: [{File, Sound}, ...]} dict."""
        if self._motions_cache is None:
            self._motions_cache = self._model.GetMotions()
        return self._motions_cache

    def GetSoundPath(self, group: str, index: int) -> str:
        key = (group, index)
        if key not in self._sound_cache:
            motions = self.GetMotions()
            entries = motions.get(group, [])
            if 0 <= index < len(entries):
                self._sound_cache[key] = entries[index].get("Sound", "")
            else:
                self._sound_cache[key] = ""
        return self._sound_cache[key]

    # ---- expression ----

    def SetExpression(self, expressionId: str, fadeout: int = -1):
        if fadeout >= 0:
            self._fadeout = fadeout
            self._expStartedAt = int(time.time() * 1000)
        else:
            self._lastExpression = expressionId
        self._model.SetExpression(expressionId)

    def SetRandomExpression(self, fadeout: int = -1) -> str:
        expId = self._model.SetRandomExpression()
        if fadeout >= 0:
            self._fadeout = fadeout
            self._expStartedAt = int(time.time() * 1000)
        else:
            self._lastExpression = expId or ""
        return expId or ""

    def ResetExpression(self):
        self._fadeout = -1
        self._expStartedAt = -1
        self._lastExpression = ""
        self._model.ResetExpression()

    def ResetExpressions(self):
        self._fadeout = -1
        self._expStartedAt = -1
        self._lastExpression = ""
        self._model.ResetExpressions()

    def AddExpression(self, expressionId: str):
        self._model.AddExpression(expressionId)

    def RemoveExpression(self, expressionId: str):
        self._model.RemoveExpression(expressionId)

    def LoadExtraExpression(self, expressionId: str, filePath: str):
        self._model.LoadExtraExpression(expressionId, filePath)

    def GetExpressionIds(self) -> list:
        return self._model.GetExpressions()

    # ---- parameters ----

    def GetParameterCount(self) -> int:
        return self._model.GetParameterCount()

    def GetParameter(self, index: int) -> Parameter:
        ids = self._model.GetParameterIds()
        param = Parameter()
        if 0 <= index < len(ids):
            param.id = ids[index]
            param.type = 0
            param.value = self._model.GetParameterValue(index)
            param.max = self._model.GetParameterMaximumValue(index)
            param.min = self._model.GetParameterMinimumValue(index)
            param.default = self._model.GetParameterDefaultValue(index)
        return param

    def GetParamIds(self) -> list:
        return self._model.GetParameterIds()

    def GetParameterValue(self, index: int) -> float:
        return self._model.GetParameterValue(index)

    def SetParameterValue(self, paramId: str, value: float, weight: float = 1.0):
        self._model.SetAndSaveParameterValueById(paramId, value, weight)

    def SetIndexParamValue(self, index: int, value: float, weight: float = 1.0):
        self._model.SetAndSaveParameterValue(index, value, weight)

    def AddParameterValue(self, paramId: str, value: float):
        self._model.AddAndSaveParameterValueById(paramId, value)

    def AddIndexParamValue(self, index: int, value: float):
        self._model.AddAndSaveParameterValue(index, value)

    def ResetParameters(self):
        self._model.ResetAllParameters()

    # ---- transform ----

    def SetOffset(self, dx: float, dy: float):
        self._offset_x, self._offset_y = dx, dy
        self._model.SetOffset(dx, dy)

    def SetOffsetX(self, sx: float):
        self._offset_x = sx
        self._model.SetOffset(sx, self._offset_y)

    def SetOffsetY(self, sy: float):
        self._offset_y = sy
        self._model.SetOffset(self._offset_x, sy)

    def SetScale(self, scale: float):
        self._model.SetScale(scale)

    def SetScaleX(self, scale: float):
        self._model.SetScaleX(scale)

    def SetScaleY(self, scale: float):
        self._model.SetScaleY(scale)

    def Rotate(self, deg: float):
        self._model.Rotate(deg)

    def Drag(self, x: float, y: float):
        self._model.Drag(x, y)

    # ---- parts ----

    def GetPartCount(self) -> int:
        return self._model.GetPartCount()

    def GetPartIds(self) -> list:
        return self._model.GetPartIds()

    def GetPartId(self, index: int) -> str:
        ids = self._model.GetPartIds()
        return ids[index] if 0 <= index < len(ids) else ""

    def SetPartOpacity(self, index: int, opacity: float):
        self._model.SetPartOpacity(index, opacity)

    def SetPartMultiplyColor(self, index: int, r: float, g: float, b: float, a: float):
        self._model.SetPartMultiplyColor(index, r, g, b, a)

    def SetPartScreenColor(self, index: int, r: float, g: float, b: float, a: float):
        self._model.SetPartScreenColor(index, r, g, b, a)

    def GetPartMultiplyColor(self, index: int):
        return self._model.GetPartMultiplyColor(index)

    def GetPartScreenColor(self, index: int):
        return self._model.GetPartScreenColor(index)

    # ---- drawables ----

    def GetDrawableIds(self) -> list:
        return self._model.GetDrawableIds()

    def SetDrawableMultiplyColor(self, index: int, r: float, g: float, b: float, a: float):
        self._model.SetDrawableMultiplyColor(index, r, g, b, a)

    def SetDrawableScreenColor(self, index: int, r: float, g: float, b: float, a: float):
        self._model.SetDrawableScreenColor(index, r, g, b, a)

    # ---- hit test ----

    def HitTest(self, hitAreaName: str, x: float, y: float) -> bool:
        return self._model.IsAreaHit(hitAreaName, x, y)

    def HitPart(self, x: float, y: float, topOnly: bool = False) -> list:
        return self._model.HitPart(x, y, topOnly)

    def HasMocConsistencyFromFile(self, mocFileName: str) -> bool:
        return self._model.HasMocConsistencyFromFile(mocFileName)

    # ---- auto features ----

    def SetAutoBreathEnable(self, enable: bool):
        self._model.SetAutoBreath(enable)

    def SetAutoBlinkEnable(self, enable: bool):
        self._model.SetAutoBlink(enable)

    # ---- canvas info ----

    def GetCanvasSize(self):
        return self._model.GetCanvasSize()

    def GetCanvasSizePixel(self):
        return self._model.GetCanvasSizePixel()

    def GetPixelsPerUnit(self) -> float:
        return self._model.GetPixelsPerUnit()

    # ---- pose / parameters ----

    def ResetPose(self):
        self._model.ResetPose()
