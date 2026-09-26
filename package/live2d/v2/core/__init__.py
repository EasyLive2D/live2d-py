from typing import TYPE_CHECKING

from .alive2d_model import ALive2DModel
from .live2d import Live2D
from .util import log, UtSystem
from .motion import Live2DMotion, AMotion, MotionQueueManager
from .physics import PhysicsHair
from .id import Id
if TYPE_CHECKING:
    from .live2d_gl_wrapper import Live2DGLWrapper
    from .live2d_model_opengl import Live2DModelOpenGL


def __getattr__(name):
    if name == "Live2DGLWrapper":
        from .live2d_gl_wrapper import Live2DGLWrapper
        globals()[name] = Live2DGLWrapper
        return Live2DGLWrapper
    if name == "Live2DModelOpenGL":
        from .live2d_model_opengl import Live2DModelOpenGL
        globals()[name] = Live2DModelOpenGL
        return Live2DModelOpenGL
    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")
