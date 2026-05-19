import math
import os.path
import time

import glfw
# import live2d.v3 as live2d
# import live2d.v2 as live2d
import live2d.v2cpp as live2d
import resources

if live2d.LIVE2D_VERSION == 3:
    from live2d.v3 import StandardParams
else:
    from live2d.v2 import StandardParams
from live2d.utils import log
from live2d.utils.lipsync import WavHandler

live2d.enableLog(True)
live2d.setLogLevel(live2d.Live2DLogLevels.LV_DEBUG)


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

    model = live2d.LAppModel()
    if live2d.LIVE2D_VERSION == 3:
        model.LoadModelJson(os.path.join(resources.RESOURCES_DIRECTORY, "v3/llny/llny.model3.json"))
    else:
        model.LoadModelJson(os.path.join(resources.RESOURCES_DIRECTORY, "v2/haru/haru.model.json"))

    model.Resize(*display)

    # Disable auto effects
    model.SetAutoBlinkEnable(False)
    model.SetAutoBreathEnable(False)

    wavHandler = WavHandler()
    lipSyncN = 3
    audioPlayed = False

    def on_start_motion_callback(group, no):
        log.Info("start motion: [%s_%d]" % (group, no))

    def on_finish_motion_callback():
        log.Info("motion finished")

    # Print all parameters
    print(f"Parameter Count: {model.GetParameterCount()}")
    for i in range(model.GetParameterCount()):
        param = model.GetParameter(i)
        log.Debug(param.id, param.type, param.value, param.max, param.min, param.default)

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
        elif key == glfw.KEY_E:
            model.ResetExpression()
    glfw.set_key_callback(window, on_key)

    def on_mouse_button(window, button, action, mods):
        nonlocal currentTopClickedPartId
        if button == glfw.MOUSE_BUTTON_LEFT and action == glfw.PRESS:
            x, y = glfw.get_cursor_pos(window)
            currentTopClickedPartId = getHitFeedback(x, y)
            model.SetRandomExpression()
            model.StartRandomMotion(priority=3, onFinishMotionHandler=on_finish_motion_callback)
    glfw.set_mouse_button_callback(window, on_mouse_button)

    def on_cursor_pos(window, x, y):
        nonlocal currentTopClickedPartId
        model.Drag(x, y)
        currentTopClickedPartId = getHitFeedback(x, y)
    glfw.set_cursor_pos_callback(window, on_cursor_pos)

    glfw.swap_interval(0)

    # Rotate animation
    radius_per_frame = math.pi * 10 / 1000 * 0.5
    deg_max = 5
    progress = 0

    fps_frames = 0
    fps_timer = time.time()

    while not glfw.window_should_close(window):
        glfw.poll_events()

        progress += radius_per_frame
        deg = math.sin(progress) * deg_max
        model.Rotate(deg)

        model.Update()

        if currentTopClickedPartId is not None:
            pidx = partIds.index(currentTopClickedPartId)
            model.SetPartOpacity(pidx, 0.5)
            model.SetPartMultiplyColor(pidx, 0.0, 0.0, 1.0, 0.9)

        if wavHandler.Update():
            model.SetParameterValue(StandardParams.ParamMouthOpenY, wavHandler.GetRms() * lipSyncN)

        if not audioPlayed:
            model.StartMotion("", 0, live2d.MotionPriority.FORCE,
                              on_start_motion_callback, on_finish_motion_callback)
            audioPlayed = True

        model.SetOffset(dx, dy)
        model.SetScale(scale)
        live2d.clearBuffer(1.0, 0.0, 0.0, 0.0)
        model.Draw()
        glfw.swap_buffers(window)

        fps_frames += 1
        if time.time() - fps_timer >= 1.0:
            glfw.set_window_title(window, f"glfw - FPS: {fps_frames}")
            fps_frames = 0
            fps_timer = time.time()

    live2d.dispose()
    glfw.terminate()


if __name__ == "__main__":
    main()
