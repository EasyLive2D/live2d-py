from . import _v2cpp
from ._v2cpp import LAppModel

# Re-export module-level functions from C++ extension
init = _v2cpp.init
glInit = _v2cpp.glInit
glRelease = _v2cpp.glRelease
dispose = _v2cpp.dispose
clearBuffer = _v2cpp.clearBuffer
enableLog = _v2cpp.enableLog
isLogEnabled = _v2cpp.isLogEnabled
setLogLevel = _v2cpp.setLogLevel
getLogLevel = _v2cpp.getLogLevel


class MotionPriority:
    NONE = 0
    IDLE = 1
    NORMAL = 2
    FORCE = 3


class MotionGroup:
    IDLE = "idle"
    TAP_BODY = "tap_body"
    FLICK_HEAD = "flick_head"
    PINCH_IN = "pinch_in"
    PINCH_OUT = "pinch_out"
    SHAKE = "shake"


class HitArea:
    HEAD = "head"
    BODY = "body"


class StandardParams:
    ParamAngleX = "PARAM_ANGLE_X"
    ParamAngleY = "PARAM_ANGLE_Y"
    ParamAngleZ = "PARAM_ANGLE_Z"
    ParamEyeLOpen = "PARAM_EYE_L_OPEN"
    ParamEyeROpen = "PARAM_EYE_R_OPEN"
    ParamEyeLSmile = "PARAM_EYE_L_SMILE"
    ParamEyeRSmile = "PARAM_EYE_R_SMILE"
    ParamEyeBallX = "PARAM_EYE_BALL_X"
    ParamEyeBallY = "PARAM_EYE_BALL_Y"
    ParamEyeBallForm = "PARAM_EYE_BALL_FORM"
    ParamBrowLX = "PARAM_BROW_L_X"
    ParamBrowLY = "PARAM_BROW_L_Y"
    ParamBrowLAngle = "PARAM_BROW_L_ANGLE"
    ParamBrowLForm = "PARAM_BROW_L_FORM"
    ParamBrowRX = "PARAM_BROW_R_X"
    ParamBrowRY = "PARAM_BROW_R_Y"
    ParamBrowRAngle = "PARAM_BROW_R_ANGLE"
    ParamBrowRForm = "PARAM_BROW_R_FORM"
    ParamMouthOpenY = "PARAM_MOUTH_OPEN_Y"
    ParamMouthForm = "PARAM_MOUTH_FORM"
    ParamSmile = "PARAM_SMILE"
    ParamTere = "PARAM_TERE"
    ParamBodyAngleX = "PARAM_BODY_ANGLE_X"
    ParamBodyAngleZ = "PARAM_BODY_ANGLE_Z"
    ParamBreath = "PARAM_BREATH"
    ParamHairFront = "PARAM_HAIR_FRONT"
    ParamHairSide = "PARAM_HAIR_SIDE"
    ParamHairBack = "PARAM_HAIR_BACK"
    ParamHairFuwa = "PARAM_HAIR_FUWA"
    ParamShoulderX = "PARAM_SHOULDER_X"
    ParamBustX = "PARAM_BUST_X"
    ParamBustY = "PARAM_BUST_Y"
    ParamBaseX = "PARAM_BASE_X"
    ParamBaseY = "PARAM_BASE_Y"


class Parameter:
    TYPE_INNER = 0  # params defined in moc file
    TYPE_OUTER = 1  # params generated from motion file

    def __init__(self):
        self.id: str = ""
        self.type: int = 0
        self.value: float = 0
        self.max: float = 0
        self.min: float = 0
        self.default: float = 0

    def __str__(self):
        return f"Parameter(id={self.id}, type={self.type}, value={self.value}, max={self.max}, min={self.min}, default={self.default})"


class Live2DLogLevels:
    LV_DEBUG = 0
    LV_INFO = 0
    LV_WARN = 0
    LV_ERROR = 0


LIVE2D_VERSION = 2

__all__ = [
    "LAppModel",
    "MotionPriority",
    "MotionGroup",
    "HitArea",
    "StandardParams",
    "Parameter",
    "Live2DLogLevels",
    "init",
    "glInit",
    "isLogEnabled",
    "enableLog",
    "setLogLevel",
    "getLogLevel",
    "glRelease",
    "clearBuffer",
    "dispose",
    "LIVE2D_VERSION",
]
