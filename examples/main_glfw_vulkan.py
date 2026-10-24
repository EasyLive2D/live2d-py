"""Vulkan backend example (experimental).

与 OpenGL 版示例的区别：
1. 窗口 hint 使用 CLIENT_API = NO_API（不创建 GL 上下文）；
2. 不调用 live2d.glInit / clearBuffer / swap_buffers ——
   帧流程由 Python 驱动：每帧 vkBeginFrame()（acquire → 清屏 → 打开主区间）
   → model.Draw()（仅录制命令）→ vkEndFrame()（提交 → present）；
3. live2d.vkInit 由 Python 侧传入原生窗口句柄（Windows 下为 HWND），
   surface 创建由 VulkanManager 内部的 vkCreateWin32SurfaceKHR 完成，
   不需要注入 glfwCreateWindowSurface 函数指针。

仅 v3 模型经过验证。v2 的 Vulkan shader 从 CWD 的 ./FrameworkShaders 读取。
"""

import ctypes
import math
import os
import platform
import sys
import time

import glfw
from glfw.GLFW import _glfw as _glfw_lib

import live2d
import resources

from live2d.utils import log
from live2d.utils.lipsync import WavHandler

live2d.enableLog(True)
live2d.setLogLevel(live2d.LogLevels.LV_INFO)


LIVE2D_VERSION = 2
MULTI_MODEL = False

if LIVE2D_VERSION == 2:
    from live2d import StandardParamsV2 as StandardParams
else:
    from live2d import StandardParamsV3 as StandardParams


def get_native_window_handle(window):
    """从 glfw 窗口获取平台原生句柄。
    Windows 返回 HWND，其他平台未实现（TODO）。
    """
    sysname = platform.system()
    if sysname == "Windows":
        # glfwGetWin32Window(GLFWwindow*) -> HWND
        _glfw_lib.glfwGetWin32Window.restype = ctypes.c_void_p
        _glfw_lib.glfwGetWin32Window.argtypes = [ctypes.c_void_p]
        hwnd = _glfw_lib.glfwGetWin32Window(ctypes.cast(window, ctypes.c_void_p).value)
        return hwnd, 0  # (window_handle, display_handle)
    else:
        # TODO: Linux(X11/Wayland) / macOS 対応
        raise NotImplementedError(f"native window handle not implemented for {sysname}")


def main():
    if not glfw.init():
        raise RuntimeError("glfw.init failed")

    # Vulkan 窗口：不要 GL 上下文
    glfw.window_hint(glfw.CLIENT_API, glfw.NO_API)
    glfw.window_hint(glfw.RESIZABLE, True)

    display = (800, 800)
    window = glfw.create_window(*display, "live2d-py Vulkan", None, None)
    if not window:
        glfw.terminate()
        raise RuntimeError("glfw.create_window failed")

    # CubismFramework 初始化（不含 GL）；同时设置 shader dir 为包目录。
    live2d.init()

    # Vulkan 实例/设备/交换链初始化。
    # 实例扩展由 native 层 (VulkanManager::GetRequiredExtensions) 自动补齐，
    # 无需从 Python 传入；shader 经 FileUtils 从 SHADER_DIR 加载，无需 chdir。
    window_handle, display_handle = get_native_window_handle(window)
    live2d.vkInit(window_handle, display_handle)

    model = live2d.Model()
    # model.LoadModelJson(os.path.join(resources.RESOURCES_DIRECTORY, "v3/llny/llny.model3.json"))
    # model.LoadModelJson(os.path.join(resources.RESOURCES_DIRECTORY, "v3/Haru/Haru.model3.json"), create_renderer=False)
    # model.LoadModelJson(os.path.join(resources.RESOURCES_DIRECTORY, "v3/绫地宁宁/nn_min.model3.json"), create_renderer=False)
    # model.LoadModelJson(os.path.join(resources.RESOURCES_DIRECTORY, "v3/akane/InariTalk_PSDtool2.model3.json"), create_renderer=False)
    # model.LoadModelJson(os.path.join(resources.RESOURCES_DIRECTORY, "v2/托尔/model0.json"),
    # model.LoadModelJson(os.path.join(resources.RESOURCES_DIRECTORY, "v2/kasumi2/kasumi2.model.json"),
    # model.LoadModelJson(os.path.join(resources.RESOURCES_DIRECTORY, "v2/haru/haru.model.json"),
    # model.LoadModelJson(os.path.join(resources.RESOURCES_DIRECTORY, "v2/hibiki/hibiki.model.json"),
    model.LoadModelJson(os.path.join(resources.RESOURCES_DIRECTORY, "v2/shizuku/shizuku.model.json"),
    # model.LoadModelJson(os.path.join(resources.RESOURCES_DIRECTORY, "v2/kana/Kobayaxi.model.json"),
                        create_renderer=True)  # Load model without creating renderer
    model2 = None
    if MULTI_MODEL:
        model2 = live2d.Model()
        # model.LoadModelJson(os.path.join(resources.RESOURCES_DIRECTORY, "v2/托尔/model0.json"),
        model2.LoadModelJson(os.path.join(resources.RESOURCES_DIRECTORY, "v2/kasumi2/kasumi2.model.json"),
        # model.LoadModelJson(os.path.join(resources.RESOURCES_DIRECTORY, "v2/haru/haru.model.json"),
        # model.LoadModelJson(os.path.join(resources.RESOURCES_DIRECTORY, "v2/hibiki/hibiki.model.json"),
        # model.LoadModelJson(os.path.join(resources.RESOURCES_DIRECTORY, "v2/shizuku/shizuku.model.json"),
                            create_renderer=True)  # Load model without creating renderer
    model.CreateRenderer()
    model.Resize(*display)
    if MULTI_MODEL:
        model2.Resize(*display)

    # Disable auto effects
    model.SetAutoBlink(False)
    model.SetAutoBreath(False)

    wavHandler = WavHandler()
    lipSyncN = 3
    audioPlayed = False

    def on_start_motion_callback(group, no):
        log.LOGI("start motion: [%s_%d]" % (group, no))

    def on_finish_motion_callback(group, no):
        log.LOGI("motion finished")

    # Print all parameters
    print(f"Parameter Count: {model.GetParamCount()}")
    paramIds = model.GetParamIds()
    for i in range(model.GetParamCount()):
        log.LOGD(paramIds[i], 0, model.GetParamValueByIndex(i), model.GetParamMaxByIndex(i),
                 model.GetParamMinByIndex(i), model.GetParamDefaultByIndex(i))

    # Print part IDs
    partIds = model.GetPartIds()
    print(f"Part Count: {len(partIds)}")
    print("Part IDs:", partIds)
    if MULTI_MODEL:
        partIds2 = model2.GetPartIds()
        print(f"Part Count2: {len(partIds2)}")
        print("Part IDs2:", partIds2)

    print("Canvas size:", model.GetCanvasSize())
    print("Canvas size in pixels:", model.GetCanvasSizePixel())
    print("Pixels per unit:", model.GetPixelsPerUnit())

    def on_resize(_window, width, height):
        if width > 0 and height > 0:
            model.Resize(width, height)
            if MULTI_MODEL:
                model2.Resize(width, height)

    glfw.set_window_size_callback(window, on_resize)

    dx, dy = 0.0, 0.0
    scale = 1.0
    currentTopClickedPartId = None
    currentTopClickedPartId2 = None

    def getHitFeedback(x, y):
        nonlocal currentTopClickedPartId, currentTopClickedPartId2
        t = time.time()
        hitPartIds = model.HitPart(x, y, False)
        print(f"hit part cost: {time.time() - t:.4f}s")
        print(f"hit parts: {hitPartIds}")
        t = time.time()
        if MULTI_MODEL:
            hitPartIds2 = model2.HitPart(x, y, False)
            print(f"hit part2 cost: {time.time() - t:.4f}s")
            print(f"hit parts2: {hitPartIds2}")
        if currentTopClickedPartId is not None:
            pidx = partIds.index(currentTopClickedPartId)
            model.SetPartOpacity(pidx, 1)
            model.SetPartMultiplyColor(pidx, 1.0, 1.0, 1.0, 1.0)
            print("Part Multiply Color:", model.GetPartMultiplyColor(pidx))
        if MULTI_MODEL and currentTopClickedPartId2 is not None:
            pidx2 = partIds2.index(currentTopClickedPartId2)
            model2.SetPartOpacity(pidx2, 1)
            model2.SetPartMultiplyColor(pidx2, 1.0, 1.0, 1.0, 1.0)
            print("Part Multiply Color2:", model2.GetPartMultiplyColor(pidx2))
        
        ret1 = hitPartIds[0] if len(hitPartIds) > 0 else None
        if MULTI_MODEL:
            ret2 = hitPartIds2[0] if len(hitPartIds2) > 0 else None
        else:
            ret2 = None
        return ret1, ret2

    def on_key(_window, key, _scancode, action, _mods):
        nonlocal dx, dy, scale
        if action != glfw.PRESS:
            return
        if key == glfw.KEY_LEFT:
            dx -= 0.1
        elif key == glfw.KEY_RIGHT:
            dx += 0.1
        elif key == glfw.KEY_UP:
            dy += 0.1
        elif key == glfw.KEY_DOWN:
            dy -= 0.1
        elif key == glfw.KEY_I:
            scale += 0.1
        elif key == glfw.KEY_U:
            scale -= 0.1
        elif key == glfw.KEY_R:
            model.StopAllMotions()
            model.ResetPose()
            model.ResetAllParameters()
        elif key == glfw.KEY_E:
            model.ResetExpression()

    glfw.set_key_callback(window, on_key)

    def on_cursor_pos(_window, x, y):
        nonlocal currentTopClickedPartId, currentTopClickedPartId2
        model.Drag(x, y)
        if MULTI_MODEL:
            model2.Drag(x, y)

        currentTopClickedPartId, currentTopClickedPartId2 = getHitFeedback(x, y)


    glfw.set_cursor_pos_callback(window, on_cursor_pos)

    def on_mouse_button(_window, button, action, _mods):
        nonlocal currentTopClickedPartId, currentTopClickedPartId2
        if button == glfw.MOUSE_BUTTON_LEFT and action == glfw.PRESS:
            x, y = glfw.get_cursor_pos(window)
            if MULTI_MODEL:
                currentTopClickedPartId2 = getHitFeedback(x, y)[1]
            else:
                currentTopClickedPartId2 = None
            model.SetRandomExpression()
            model.StartRandomMotion(priority=3, onFinish=on_finish_motion_callback)

            if MULTI_MODEL:
                model2.SetRandomExpression()
                model2.StartRandomMotion(priority=3, onFinish=on_finish_motion_callback)

    glfw.set_mouse_button_callback(window, on_mouse_button)

    model.StartRandomMotion(priority=3)

    # Rotate animation
    inc_per_sec = math.pi * 2 / 3
    deg_max = 5
    progress = 0

    last_time = time.time()
    fps_frames = 0
    fps_timer = last_time

    # LIVE2D_VK_DURATION 设定后，运行指定秒数自动关闭（便于无头验证）
    auto_close_after = float(os.environ.get("LIVE2D_VK_DURATION", "0")) or None
    start_time = time.time()

    while not glfw.window_should_close(window):
        glfw.poll_events()

        now = time.time()
        delta = now - last_time
        last_time = now
        progress += inc_per_sec * delta
        deg = math.sin(progress) * deg_max
        model.Rotate(deg)
        if MULTI_MODEL:
            model2.Rotate(deg)

        model.Update()
        if MULTI_MODEL:
            model2.Update()

        if currentTopClickedPartId is not None:
            pidx = partIds.index(currentTopClickedPartId)
            model.SetPartOpacity(pidx, 0.5)
            model.SetPartMultiplyColor(pidx, 0.0, 0.0, 1.0, 0.9)
        
        if MULTI_MODEL and currentTopClickedPartId2 is not None:
            pidx2 = partIds2.index(currentTopClickedPartId2)
            model2.SetPartOpacity(pidx2, 0.5)
            model2.SetPartMultiplyColor(pidx2, 0.0, 0.0, 1.0, 0.9)

        if wavHandler.Update():
            model.SetParamById(StandardParams.ParamMouthOpenY, wavHandler.GetRms() * lipSyncN)
            if MULTI_MODEL:
                model2.SetParamById(StandardParams.ParamMouthOpenY, wavHandler.GetRms() * lipSyncN)

        if not audioPlayed:
            model.StartMotion("", 0, live2d.MotionPriority.FORCE,
                              on_start_motion_callback, on_finish_motion_callback)
            audioPlayed = True

        model.SetOffset(dx, dy)
        model.SetScale(scale)
        if MULTI_MODEL:
            model2.SetScale(scale)
            model2.SetOffset(-dx, -dy)
        # 帧解耦：BeginFrame 打开主区间并清屏，Draw 仅录制命令，
        # 多模型/自定义绘制可在各 Draw() 之间直接向 cmd 注入命令
        frame = live2d.vkBeginFrame()
        if frame is not None:
            _cmd, _frame_index, width, height = frame
            if MULTI_MODEL:
                model2.Draw()
            model.Draw()
            live2d.vkEndFrame()

        if model.IsMotionFinished():
            model.StartRandomMotion(priority=3)

        fps_frames += 1
        if now - fps_timer >= 1.0:
            glfw.set_window_title(window, f"live2d-py Vulkan - FPS: {fps_frames}")
            fps_frames = 0
            fps_timer = now

        if auto_close_after is not None and now - start_time >= auto_close_after:
            glfw.set_window_should_close(window, True)

    model.DestroyRenderer()
    if MULTI_MODEL:
        model2.DestroyRenderer()
    live2d.vkDestroy()
    live2d.dispose()
    glfw.terminate()


if __name__ == "__main__":
    main()
