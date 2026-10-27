"""Vulkan 压测程序（与 bench_gl.py 对等，供 tests/tracy_bench.py 调用）。

帧流程全部由 Python 驱动：vkBeginFrame -> Draw -> vkEndFrame，无交互、无音频。
"""

import argparse
import ctypes
import math
import os
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "package"))

# 默认 MAILBOX 会按显示器垂直同步节流（本机 165Hz -> ~330 FPS），压测需要真实吞吐，
# 因此与 bench_gl.py 的 swap_interval(0) 对齐，关闭呈现节流
os.environ.setdefault("LIVE2D_VK_PRESENT_MODE", "immediate")

import glfw
from glfw.GLFW import _glfw as _glfw_lib

import live2d


def parse_size(text):
    w, h = text.lower().split("x")
    return int(w), int(h)


def get_native_window_handle(window):
    """返回 (backend, window, display, connection)。

    - Windows:  (BACKEND_WIN32,   HWND,            None,       None)
    - X11 Xlib: (BACKEND_XLIB,    Window,         Display*,   None)
    - Wayland:  (BACKEND_WAYLAND, wl_surface*,    wl_display*, None)
    """
    import ctypes
    import platform as _platform

    # 与 C++ 侧 SurfaceBackend 枚举值保持一致
    BACKEND_WIN32   = 1
    BACKEND_XLIB    = 2
    BACKEND_XCB     = 3
    BACKEND_WAYLAND = 4

    # GLFW 3.4 平台枚举
    GLFW_PLATFORM_WAYLAND = 0x00060003   # 393219
    GLFW_PLATFORM_X11     = 0x00060004   # 393220

    # 统一把 Python 侧 window 对象转成整数指针值
    if isinstance(window, int):
        win = window
    elif isinstance(window, ctypes.c_void_p):
        win = window.value
    else:
        win = ctypes.cast(window, ctypes.c_void_p).value

    sysname = _platform.system()

    # ---------------- Windows ----------------
    if sysname == "Windows":
        _glfw_lib.glfwGetWin32Window.restype = ctypes.c_void_p
        _glfw_lib.glfwGetWin32Window.argtypes = [ctypes.c_void_p]
        hwnd = _glfw_lib.glfwGetWin32Window(win)
        if not hwnd:
            raise RuntimeError("failed to get HWND from GLFW")
        return BACKEND_WIN32, hwnd, 0, 0

    # ---------------- Linux ----------------
    if sysname == "Linux":
        # 判断 GLFW 实际初始化的后端；GLFW < 3.4 在 Linux 上只有 X11
        if hasattr(_glfw_lib, "glfwGetPlatform"):
            _glfw_lib.glfwGetPlatform.restype = ctypes.c_int
            _glfw_lib.glfwGetPlatform.argtypes = []
            plat = _glfw_lib.glfwGetPlatform()
        else:
            plat = GLFW_PLATFORM_X11

        # Wayland
        if plat == GLFW_PLATFORM_WAYLAND:
            if not hasattr(_glfw_lib, "glfwGetWaylandWindow"):
                raise RuntimeError(
                    "GLFW reports Wayland but glfwGetWaylandWindow is missing "
                    "(GLFW built without Wayland support?)"
                )
            _glfw_lib.glfwGetWaylandWindow.restype = ctypes.c_void_p
            _glfw_lib.glfwGetWaylandWindow.argtypes = [ctypes.c_void_p]
            _glfw_lib.glfwGetWaylandDisplay.restype = ctypes.c_void_p
            _glfw_lib.glfwGetWaylandDisplay.argtypes = []

            wl_surface = _glfw_lib.glfwGetWaylandWindow(win)
            wl_display = _glfw_lib.glfwGetWaylandDisplay()
            if not wl_surface or not wl_display:
                raise RuntimeError("failed to get Wayland handles from GLFW")
            return BACKEND_WAYLAND, wl_surface, wl_display, 0

        # X11（GLFW 默认走 Xlib）
        if plat == GLFW_PLATFORM_X11:
            _glfw_lib.glfwGetX11Window.restype = ctypes.c_ulong
            _glfw_lib.glfwGetX11Window.argtypes = [ctypes.c_void_p]
            _glfw_lib.glfwGetX11Display.restype = ctypes.c_void_p
            _glfw_lib.glfwGetX11Display.argtypes = []

            xwin = _glfw_lib.glfwGetX11Window(win)
            display = _glfw_lib.glfwGetX11Display()
            if not xwin or not display:
                raise RuntimeError("failed to get X11 handles from GLFW")
            return BACKEND_XLIB, xwin, display, 0

        raise RuntimeError(f"unsupported GLFW platform on Linux: {plat}")

    raise NotImplementedError(f"native window handle not implemented for {sysname}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--model", required=True, help="模型 json 路径")
    ap.add_argument("--size", default="800x800", help="窗口尺寸，默认 800x800")
    ap.add_argument("--seconds", type=float, default=0, help="运行秒数，0=一直跑")
    args = ap.parse_args()
    width, height = parse_size(args.size)

    live2d.enableLog(True)
    live2d.setLogLevel(live2d.LogLevels.LV_INFO)  # 输出 Device 行，供压测脚本记录 GPU

    if not glfw.init():
        raise RuntimeError("glfw.init failed")
    glfw.window_hint(glfw.CLIENT_API, glfw.NO_API)
    glfw.window_hint(glfw.RESIZABLE, False)
    window = glfw.create_window(width, height, "live2d-py VK bench", None, None)
    if not window:
        glfw.terminate()
        raise RuntimeError("glfw.create_window failed")

    live2d.init()
    live2d.vkInit(*get_native_window_handle(window))

    model = live2d.Model()
    model.LoadModelJson(args.model, create_renderer=False)
    model.CreateRenderer()
    model.Resize(width, height)
    model.SetAutoBlink(True)
    model.SetAutoBreath(True)
    model.StartMotion("", 0, live2d.MotionPriority.FORCE)

    progress = 0.0
    last = time.time()
    start = last
    while not glfw.window_should_close(window):
        glfw.poll_events()

        now = time.time()
        progress += math.pi * 2 / 3 * (now - last)
        last = now

        model.Rotate(math.sin(progress) * 5)
        model.Update()

        frame = live2d.vkBeginFrame()
        if frame is not None:
            model.Draw()
            live2d.vkEndFrame()

        if args.seconds and now - start >= args.seconds:
            break

    model.DestroyRenderer()
    live2d.vkDestroy()
    live2d.dispose()
    glfw.terminate()


if __name__ == "__main__":
    main()
