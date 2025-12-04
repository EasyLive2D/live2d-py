from .params import *  # pyinstaller may not find it (hidden import)
from .live2d import *


class MotionPriority:
    NONE = 0
    IDLE = 1
    NORMAL = 2
    FORCE = 3


class MotionGroup:
    IDLE = "Idle"
    TAP_HEAD = "TapHead"


class HitArea:
    HEAD = MotionGroup.TAP_HEAD


LIVE2D_VERSION = 3


def init():
    import os
    __cd = os.path.split(__file__)[0]
    init_internal(str(__cd))