import resources
import math
import os
import os.path
import sys
import time

if sys.platform.startswith("linux") and not os.environ.get("PYOPENGL_PLATFORM"):
    # glfw 创建的是 GLX 上下文；PyOpenGL 在本机默认选 EGL 会导致 context 跟踪失效
    os.environ["PYOPENGL_PLATFORM"] = "glx"

import glfw
import live2d

from live2d.utils import log
from live2d.utils.lipsync import WavHandler

import OpenGL.GL as GL

live2d.enableLog(True)
live2d.setLogLevel(live2d.LogLevels.LV_DEBUG)

LOAD_FROM_JSON = True
LIVE2D_VERSION = 2

if LIVE2D_VERSION == 2:
    from live2d import StandardParamsV2 as StandardParams
else:
    from live2d import StandardParamsV3 as StandardParams


def load_from_json_string():
    from live2d.utils.model_json import Motion, ModelJson
    m = ModelJson(version=3)
    m.model = "Haru.moc3"
    m.textures = [
        "Haru.2048/texture_00.png",
        "Haru.2048/texture_01.png",
    ]
    m.physics = "Haru.physics3.json"
    m.pose = "Haru.pose3.json"
    m.display_info = "Haru.cdi3.json"
    m.user_data = "Haru.userdata3.json"

    for i in range(1, 3):
        m.add_expression(f"F{i:02d}", f"expressions/F{i:02d}.exp3.json")

    m.add_motion("Idle", Motion("motions/haru_g_idle.motion3.json", 0.5, 0.5))
    m.add_motion(
        "TapBody",
        Motion(
            "motions/haru_g_m26.motion3.json",
            0.5,
            0.5,
            sound="sounds/haru_talk_13.wav",
        ),
    )

    m.add_group("Parameter", "EyeBlink", ["ParamEyeLOpen", "ParamEyeROpen"])
    m.add_group("Parameter", "LipSync", ["ParamMouthOpenY"])
    m.add_hit_area("HitArea", "Head")
    m.add_hit_area("HitArea2", "Body")

    model = live2d.Model()
    model.LoadFromJsonString(m.to_string(), root_path=resources.RESOURCES_DIRECTORY + "/v3/Haru", create_renderer=False)
    return model


def main():
    if not glfw.init():
        raise RuntimeError("glfw.init failed")

    display = (500, 600)
    window = glfw.create_window(*display, "glfw", None, None)
    if not window:
        glfw.terminate()
        return
    glfw.make_context_current(window)

    live2d.init()
    live2d.glInit()

    model = None
    if LOAD_FROM_JSON:
        model = load_from_json_string()
    elif LIVE2D_VERSION == 3:
        model = live2d.Model()
        # model.LoadModelJson(os.path.join(resources.RESOURCES_DIRECTORY, "v3/llny/llny.model3.json"))
        model.LoadModelJson(os.path.join(resources.RESOURCES_DIRECTORY, "v3/haru/haru.model3.json"), create_renderer=False)
    elif LIVE2D_VERSION == 2:
        model = live2d.Model()
        model.LoadModelJson(os.path.join(resources.RESOURCES_DIRECTORY, "v2/haru/haru.model.json"),
                            create_renderer=False)  # Load model without creating renderer
    model.CreateRenderer()

    model.Resize(*display)

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

    print("Canvas size:", model.GetCanvasSize())
    print("Canvas size in pixels:", model.GetCanvasSizePixel())
    print("Pixels per unit:", model.GetPixelsPerUnit())

    # ---- Keyboard / drag state ----
    dx, dy = 0.0, 0.0
    scale = 1.0
    currentTopClickedPartId = None

    def getHitFeedback(x, y):
        nonlocal currentTopClickedPartId
        t = time.time()
        hitPartIds = model.HitPart(x, y, False)
        print(f"hit part cost: {time.time() - t:.4f}s")
        print(f"hit parts: {hitPartIds}")
        if currentTopClickedPartId is not None:
            pidx = partIds.index(currentTopClickedPartId)
            model.SetPartOpacity(pidx, 1)
            model.SetPartMultiplyColor(pidx, 1.0, 1.0, 1.0, 1.0)
            print("Part Multiply Color:", model.GetPartMultiplyColor(pidx))
        if len(hitPartIds) > 0:
            return hitPartIds[0]

    def on_key(window, key, scancode, action, mods):
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

    def on_mouse_button(window, button, action, mods):
        nonlocal currentTopClickedPartId
        if button == glfw.MOUSE_BUTTON_LEFT and action == glfw.PRESS:
            x, y = glfw.get_cursor_pos(window)
            currentTopClickedPartId = getHitFeedback(x, y)
            model.SetRandomExpression()
            model.StartRandomMotion(priority=3, onFinish=on_finish_motion_callback)
    glfw.set_mouse_button_callback(window, on_mouse_button)

    def on_cursor_pos(window, x, y):
        nonlocal currentTopClickedPartId
        model.Drag(x, y)
        currentTopClickedPartId = getHitFeedback(x, y)
    glfw.set_cursor_pos_callback(window, on_cursor_pos)

    def on_resize(window, width, height):
        GL.glViewport(0, 0, width, height)
        model.Resize(width, height)
    glfw.set_window_size_callback(window, on_resize)

    glfw.swap_interval(0)

    # Rotate animation
    inc_per_sec = math.pi * 2 / 3
    deg_max = 5
    progress = 0
    last_time = time.time()

    fps_frames = 0
    fps_timer = last_time

    glfw.swap_interval(1)

    model.DestroyRenderer()
    model.CreateRenderer()

    while not glfw.window_should_close(window):
        glfw.poll_events()
        now = time.time()
        progress += inc_per_sec * (now - last_time)
        last_time = now
        deg = math.sin(progress) * deg_max
        model.Rotate(deg)

        model.Update()

        if currentTopClickedPartId is not None:
            pidx = partIds.index(currentTopClickedPartId)
            model.SetPartOpacity(pidx, 0.5)
            model.SetPartMultiplyColor(pidx, 0.0, 0.0, 1.0, 0.9)

        if wavHandler.Update():
            model.SetParamById(StandardParams.ParamMouthOpenY, wavHandler.GetRms() * lipSyncN)

        if not audioPlayed:
            model.StartMotion("", 0, live2d.MotionPriority.FORCE,
                              on_start_motion_callback, on_finish_motion_callback)
            audioPlayed = True

        model.SetOffset(dx, dy)
        model.SetScale(scale)
        live2d.clearBuffer(1.0, 0.0, 0.0, 0.0)

        GL.glPushAttrib(GL.GL_CURRENT_BIT | GL.GL_ENABLE_BIT | GL.GL_POLYGON_BIT | GL.GL_COLOR_BUFFER_BIT)
        GL.glUseProgram(0)
        GL.glDisable(GL.GL_DEPTH_TEST)
        GL.glDisable(GL.GL_CULL_FACE)
        GL.glEnable(GL.GL_BLEND)
        GL.glBlendFuncSeparate(GL.GL_ONE, GL.GL_ONE_MINUS_SRC_ALPHA, GL.GL_ONE, GL.GL_ONE_MINUS_SRC_ALPHA)

        model.Draw()
        GL.glPopAttrib()

        glfw.swap_buffers(window)

        fps_frames += 1
        if time.time() - fps_timer >= 1.0:
            glfw.set_window_title(window, f"glfw - FPS: {fps_frames}")
            fps_frames = 0
            fps_timer = time.time()

    live2d.dispose()

    # if crashes when exiting, try to explicitly destroy the renderer before terminating glfw
    # this is a workaround for a known issue in some environments where the OpenGL context is not properly released
    glfw.terminate()


if __name__ == "__main__":
    main()
