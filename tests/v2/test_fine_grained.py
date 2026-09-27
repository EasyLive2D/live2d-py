"""
fine-grained model example — v2 (v2cpp) version

照抄 examples/main_glfw_fine_grained.py，仅两处替换:
  1. import live2d.v2cpp as live2d / 类名 LAppModel（v3 示例为 live2d.v3 / Model）
  2. 模型与额外动作/表情资源换成 v2 (kasumi2)

运行（repo 根目录）:
  python tests/v2/test_fine_grained.py
"""
import os
import random
import sys
import time

# 让 live2d 包可导入（v3 示例依赖 pip 安装，这里直接指向 package/）
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "package"))

import live2d

RESOURCES = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "Resources")

# initialize memory allocation for live2d
live2d.init()

model = live2d.Model()
# LoadModelJson can be called without an OpenGL context
# （v2cpp 默认 create_renderer=True 会在加载时创建渲染器，需要 GL 上下文，
#   与 v3 示例不同——v3 的渲染器由后面的 CreateRenderer(2) 创建，所以这里显式关掉）
model.LoadModelJson(
    os.path.join(RESOURCES, "v2/kasumi2/kasumi2.model.json"),
    create_renderer=False,
)

# load extra motion files not defined in model.json
no1 = model.LoadExtraMotion(
    "extra",
    os.path.join(RESOURCES, "v2/kasumi2/live2d/001_live_event_47_ssr_idle01.mtn"),
)
print("Loaded motion index is", no1)

no2 = model.LoadExtraMotion(
    "extra",
    os.path.join(RESOURCES, "v2/kasumi2/live2d/001_live_event_47_ssr_idle02.mtn"),
)
print("Loaded motion index is", no2)

# Get Basic Model Info
print("model home dir:", model.GetModelHomeDir())
print("param ids:", model.GetParamIds())
print("part ids:", model.GetPartIds())
print("drawable ids:", model.GetDrawableIds())
print("expressions:", model.GetExpressions())

model.LoadExtraExpression(
    "extra_0",
    os.path.join(model.GetModelHomeDir(), "live2d/001_general_angry01.exp"),
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
window = glfw.create_window(*display, "fine-grained model (v2cpp)", None, None)
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
