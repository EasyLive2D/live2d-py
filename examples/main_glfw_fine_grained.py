"""
fine-grained model example — glfw version

class: live2d.v3.Model
"""

import live2d.v3 as live2d
import resources
import os
import random
import time

# initialize memory allocation for live2d
live2d.init()

model = live2d.Model()
# LoadModelJson can be called without an OpenGL context
model.LoadModelJson(
    os.path.join(resources.RESOURCES_DIRECTORY, "v3/Haru/Haru.model3.json")
)

# load extra motion file not defined in model3.json
no1 = model.LoadExtraMotion(
    "extra",
    os.path.join(resources.RESOURCES_DIRECTORY, "v3/public_motions/drag_down.motion3.json"),
)
print("Loaded motion index is", no1)

no2 = model.LoadExtraMotion(
    "extra",
    os.path.join(resources.RESOURCES_DIRECTORY, "v3/public_motions/touch_head.motion3.json"),
)
print("Loaded motion index is", no2)

# Get Basic Model Info
print("model home dir:", model.GetModelHomeDir())
print("param ids:", model.GetParameterIds())
print("part ids:", model.GetPartIds())
print("drawable ids:", model.GetDrawableIds())
print("expressions:", model.GetExpressions())

model.LoadExtraExpression(
    "extra_0",
    os.path.join(resources.RESOURCES_DIRECTORY, "v3/public_expressions/F03.exp3.json"),
)

print("motions:", model.GetMotions())
print("canvas size:", model.GetCanvasSize())
print("canvas size in pixels:", model.GetCanvasSizePixel())
print("pixels per unit:", model.GetPixelsPerUnit())

# ---- GLFW window ----
import glfw
if not glfw.init():
    raise RuntimeError("glfw.init failed")
display = (500, 700)
window = glfw.create_window(*display, "fine-grained model", None, None)
if not window:
    glfw.terminate()
    exit()
glfw.make_context_current(window)

model.Resize(*display)
live2d.glInit()
model.CreateRenderer(2)

expressions = model.GetExpressions()
expressions.append("extra_0")
lastExpressionId = ""
activeExpressions = []

def addRandomExpression(drop_last: bool = False) -> str:
    global lastExpressionId, expressions, activeExpressions
    if drop_last:
        model.RemoveExpression(lastExpressionId)
    expId = random.choice(expressions)
    model.AddExpression(expId)
    lastExpressionId = expId
    activeExpressions.append(expId)
    return expId

offsetX = 0.0
offsetY = 0.0
scale = 1.0
degrees = 0.0

def on_key(window, key, scancode, action, mods):
    global offsetX, offsetY, scale, degrees
    if action != glfw.PRESS:
        return
    if key == glfw.KEY_UP:
        offsetY += 0.1; model.SetOffset(offsetX, offsetY)
    elif key == glfw.KEY_DOWN:
        offsetY -= 0.1; model.SetOffset(offsetX, offsetY)
    elif key == glfw.KEY_LEFT:
        offsetX -= 0.1; model.SetOffset(offsetX, offsetY)
    elif key == glfw.KEY_RIGHT:
        offsetX += 0.1; model.SetOffset(offsetX, offsetY)
    elif key == glfw.KEY_U:
        scale -= 0.1; model.SetScale(scale)
    elif key == glfw.KEY_I:
        scale += 0.1; model.SetScale(scale)
    elif key == glfw.KEY_RIGHT_BRACKET:
        degrees -= 5; model.Rotate(degrees)
    elif key == glfw.KEY_LEFT_BRACKET:
        degrees += 5; model.Rotate(degrees)
    elif key == glfw.KEY_E:
        model.StartMotion("extra", 0, 3,
                          onStart=lambda g, n: print(f"{g} {n} started"),
                          onFinish=lambda g, n: print(f"{g} {n} finished"))
    elif key == glfw.KEY_R:
        model.ResetExpressions()
    elif key == glfw.KEY_T:
        print("set expression:", model.SetRandomExpression())
    elif key == glfw.KEY_Q:
        model.ResetExpression()
glfw.set_key_callback(window, on_key)

def on_mouse_button(window, button, action, mods):
    if button == glfw.MOUSE_BUTTON_LEFT and action == glfw.PRESS:
        model.StartRandomMotion(
            onStart=lambda g, n: print(f"{g} {n} started"),
            onFinish=lambda g, n: print(f"{g} {n} finished"),
        )
glfw.set_mouse_button_callback(window, on_mouse_button)

def on_scroll(window, xoff, yoff):
    x, y = glfw.get_cursor_pos(window)
    hitDrawableIds = model.HitDrawable(x, y, True)
    print("hit drawables:", hitDrawableIds)
    hitPartIds = model.HitPart(x, y, True)
    print("hit parts:", hitPartIds)
    if model.IsAreaHit("Head", x, y):
        print("add expression:", addRandomExpression())
glfw.set_scroll_callback(window, on_scroll)

def on_cursor_pos(window, x, y):
    model.Drag(x, y)
glfw.set_cursor_pos_callback(window, on_cursor_pos)

glfw.swap_interval(1)
lastUpdateTime = time.time()

while not glfw.window_should_close(window):
    glfw.poll_events()

    live2d.clearBuffer()
    ct = time.time()
    deltaSecs = max(0.0001, ct - lastUpdateTime)
    lastUpdateTime = ct

    # === LAppModel.Update() equivalent ===
    motionUpdated = False
    model.LoadParameters()

    if not model.IsMotionFinished():
        motionUpdated = model.UpdateMotion(deltaSecs)

    model.SaveParameters()

    if not motionUpdated:
        model.UpdateBlink(deltaSecs)

    model.UpdateExpression(deltaSecs)
    model.UpdateDrag(deltaSecs)
    model.UpdateBreath(deltaSecs)
    model.UpdatePhysics(deltaSecs)
    model.UpdatePose(deltaSecs)
    # === end Update ===

    model.Draw()
    glfw.swap_buffers(window)

live2d.dispose()
glfw.terminate()
