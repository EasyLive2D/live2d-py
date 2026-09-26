from .clip_context import ClipContext
from .clip_draw_context import ClipDrawContext
from .clip_matrix import ClipMatrix
from .clip_rectf import ClipRectF
from .texture_info import TextureInfo
from .draw_param import DrawParam


def __getattr__(name):
    if name == "GLRenderer":
        from .gl_renderer import GLRenderer
        globals()[name] = GLRenderer
        return GLRenderer
    if name == "ClippingManagerOpenGL":
        from .clipping_manager_opengl import ClippingManagerOpenGL
        globals()[name] = ClippingManagerOpenGL
        return ClippingManagerOpenGL
    if name == "DrawParamOpenGL":
        from .gl_renderer import GLRenderer
        globals()[name] = GLRenderer
        return GLRenderer
    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")
