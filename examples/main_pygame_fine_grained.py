"""
fine-grained model example

class: live2d.v3.Model

API: package/live2d/v3/live2d.pyi # class Model
"""

import live2d.v3 as live2d
import resources
import os
import random


# initialize memory allocation for live2d
live2d.init()

model = live2d.Model()
# LoadModelJson can be called without an OpenGL context
# because it only reads the model3.json file, motion3.json, exp3.json and moc3 files
model.LoadModelJson(
    # os.path.join(resources.RESOURCES_DIRECTORY, "v3/小九/小九皮套（紫）/小九.model3.json")
    os.path.join(resources.RESOURCES_DIRECTORY, "v3/Haru/Haru.model3.json")
)

# load extra motion file which is not defined in model3.json
no1 = model.LoadExtraMotion(
    "extra",
    os.path.join(
        resources.RESOURCES_DIRECTORY, "v3/public_motions/drag_down.motion3.json"
    ),
)
print("Loaded motion index is", no1)

no2 = model.LoadExtraMotion(
    "extra",
    os.path.join(
        resources.RESOURCES_DIRECTORY, "v3/public_motions/touch_head.motion3.json"
    ),
)
print("Loaded motion index is", no2)

# Get Basic Model Info
modelDir = model.GetModelHomeDir()
print(modelDir)

paramIds = model.GetParameterIds()
print(paramIds)

partIds = model.GetPartIds()
print(partIds)

# the relation between part and drawable
# part[drawable1, drawable2, ...]
drawableIds = model.GetDrawableIds()
print(drawableIds)

expressions = model.GetExpressions()
print(expressions)

model.LoadExtraExpression(
    "extra_0", 
    os.path.join(
        resources.RESOURCES_DIRECTORY, "v3", "public_expressions/F03.exp3.json"
    )
)
expressions.append("extra_0")

# only motions that are defined in model3.json
# extra motions are not included
motions = model.GetMotions()
print(motions)

print("canvas size:", model.GetCanvasSize())
print("canvas size in pixels:", model.GetCanvasSizePixel())
print("pixels per unit:", model.GetPixelsPerUnit())

lastExpressionId = ""

import pygame
import time

pygame.init()
pygame.display.set_mode((500, 700), pygame.DOUBLEBUF | pygame.OPENGL | pygame.RESIZABLE)

model.Resize(500, 700)

# bind inner opengl functions
live2d.glInit()
# CreateRenderer should be called after OpenGL context is created and glInit is called
# maskBufferCount = 2
# more info about maskBufferCount: https://docs.live2d.com/zh-CHS/cubism-sdk-manual/ow-sdk-mask-premake/
# typically maskBufferCount = 2 is enough for most cases
model.CreateRenderer(2)

lastUpdateTime = time.time()
running = True

activeExpressions = []

def addRandomExpression(drop_last: bool = False) -> str:
    global lastExpressionId
    global expressions
    global activeExpressions

    if drop_last:
        model.RemoveExpression(lastExpressionId)

    expId = random.choice(expressions)
    model.AddExpression(expId)

    # mantain info about the used expressions
    lastExpressionId = expId
    activeExpressions.append(expId)
    return expId


offsetX = 0.0
offsetY = 0.0
scale = 1.0
degrees = 0.0
while True:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False
            break
        # hit test
        elif event.type == pygame.MOUSEWHEEL:
            x, y = pygame.mouse.get_pos()
            # to get the drawable ids being hit
            hitDrawableIds = model.HitDrawable(x, y, True)
            print("hit: ", hitDrawableIds)
            # to get the part ids being hit
            hitPartIds = model.HitPart(x, y, True)
            print("hit:", hitPartIds)
            # to test if the given area name is hit
            if model.IsAreaHit("Head", x, y):
                print("add expression: ", addRandomExpression())
            # some assertions to test if the algorithm is correct
            if len(hitDrawableIds) > 0:
                assert model.IsDrawableHit(drawableIds.index(hitDrawableIds[0]), x, y)
            if len(hitPartIds):
                assert model.IsPartHit(partIds.index(hitPartIds[0]), x, y)
        # motion
        elif event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
            model.StartRandomMotion(
                onStart=lambda group, no: print(f"{group} {no} started"),
                onFinish=lambda group, no: print(f"{group} {no} finished"),
            )
        elif event.type == pygame.MOUSEMOTION:
            x, y = pygame.mouse.get_pos()
            model.Drag(x, y)

        # model transform
        elif event.type == pygame.KEYDOWN:
            if event.key == pygame.K_UP:
                offsetY += 0.1
                model.SetOffset(offsetX, offsetY)
            elif event.key == pygame.K_DOWN:
                offsetY -= 0.1
                model.SetOffset(offsetX, offsetY)
            elif event.key == pygame.K_LEFT:
                offsetX -= 0.1
                model.SetOffset(offsetX, offsetY)
            elif event.key == pygame.K_RIGHT:
                offsetX += 0.1
                model.SetOffset(offsetX, offsetY)
            elif event.key == pygame.K_u:
                scale -= 0.1
                model.SetScale(scale)
            elif event.key == pygame.K_i:
                scale += 0.1
                model.SetScale(scale)
            elif event.key == pygame.K_RIGHTBRACKET:
                degrees -= 5
                model.Rotate(degrees)
            elif event.key == pygame.K_LEFTBRACKET:
                degrees += 5
                model.Rotate(degrees)
            elif event.key == pygame.K_e:
                model.StartMotion(
                    "extra",
                    0,
                    3,
                    onStart=lambda group, no: print(f"{group} {no} started"),
                    onFinish=lambda group, no: print(f"{group} {no} finished"),
                )
            elif event.key == pygame.K_r:
                model.ResetExpressions()
            elif event.key == pygame.K_t:
                print("set expression:", model.SetRandomExpression())
            elif event.key == pygame.K_q:
                model.ResetExpression()

    if not running:
        break

    live2d.clearBuffer()
    ct = time.time()
    # delta seconds
    deltaSecs = ct - lastUpdateTime
    deltaSecs = max(0.0001, deltaSecs)
    lastUpdateTime = ct

    # the following section is equal to LAppModel.Update()

    # === Section Start Update() ===
    # load cached parameters from last frame
    motionUpdated = False
    model.LoadParameters()  # initialize params using cached values

    if not model.IsMotionFinished():
        motionUpdated = model.UpdateMotion(deltaSecs)

    # if SetParameterValue is called here, the parameter will be saved to the cache
    # model.SetParameterValue(StandardParams.ParamAngleX, params.AngleX, 1)

    model.SaveParameters()  # save params to cache for next frame

    if not motionUpdated:
        # auto blink
        # update eye blink params if they are defined in the model3.json
        model.UpdateBlink(deltaSecs)

    model.UpdateExpression(deltaSecs)

    model.UpdateDrag(deltaSecs)

    # auto breath
    # update breath params such as ParamBodyAngleX, ParamAngleX...
    model.UpdateBreath(deltaSecs)

    # create physics effects according to current and previous param values
    # some params can be overridden by physics effects
    model.UpdatePhysics(deltaSecs)

    model.UpdatePose(deltaSecs)
    # === Section End Update() ===

    # Draw():
    #   1. update meshes according to the parameters
    #   2. draw meshes using opengl
    model.Draw()

    pygame.display.flip()

live2d.dispose()
pygame.quit()
