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

# Share pure-Python constants and types from v2 (seamless replacement)
from ..v2.lapp_define import MotionPriority, MotionGroup, HitArea
from ..v2.params import StandardParams, Parameter

class Live2DLogLevels:
    LV_DEBUG = 0
    LV_INFO = 0
    LV_WARN = 0
    LV_ERROR = 0

LIVE2D_VERSION = 2

__all__ = [
    'LAppModel', 'MotionPriority', 'MotionGroup', 'HitArea',
    'StandardParams', 'Parameter', 'Live2DLogLevels',
    'init', 'glInit', 'isLogEnabled', 'enableLog',
    'setLogLevel', 'getLogLevel', 'glRelease', 'clearBuffer', 'dispose',
    'LIVE2D_VERSION',
]
