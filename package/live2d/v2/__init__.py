from .core import Live2D, Live2DGLWrapper
from .core import log as __log
from .framework import Live2DFramework
from .lapp_define import MotionGroup, MotionPriority, HitArea
from .lapp_model import LAppModel
from .params import Parameter, StandardParams
from .platform_manager import PlatformManager


class Live2DLogLevels:
    LV_DEBUG: int = 0
    LV_INFO: int = 0
    LV_WARN: int = 0
    LV_ERROR: int = 0


def init():
    Live2D.init()
    Live2DFramework.setPlatformManager(PlatformManager())


def clearBuffer(r=0.0, g=0.0, b=0.0, a=0.0):
    Live2DGLWrapper.clearColor(r, g, b, a)
    Live2DGLWrapper.clear(Live2DGLWrapper.COLOR_BUFFER_BIT)


def enableLog(enable: bool):
    __log.enableLog(enable)


def isLogEnabled() -> bool:
    return __log.isLogEnabled()


def setLogLevel(level: int):
    __log.setLogLevel(level)


def getLogLevel() -> int:
    return __log.getLogLevel()


def glInit():
    pass


def glRelease():
    pass

def dispose():
    pass


LIVE2D_VERSION = 2

__all__ = ['LAppModel',
           'MotionPriority',
           'MotionGroup',
           "HitArea",
           "StandardParams",
           "Live2DLogLevels",
           "init",
           "glInit",
           "isLogEnabled",
           "enableLog",
           "setLogLevel",
           "getLogLevel",
           "glRelease",
           "clearBuffer",
           "dispose"]
