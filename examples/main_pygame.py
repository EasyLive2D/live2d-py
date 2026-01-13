# gdb 联合调试 live2d.so
# 需要使用 Debug 配置编译 LAppModelWrapper
# import ptvsd
# ptvsd.enable_attach(address=('127.0.0.1', 10010), redirect_output=True)
# ptvsd.wait_for_attach()

import math
import os
import time

import pygame
from pygame.locals import *

# import live2d.v3 as live2d
import live2d.v2 as live2d

if live2d.LIVE2D_VERSION == 3:
    from live2d.v3 import StandardParams
else:
    from live2d.v2 import StandardParams
from live2d.utils import log


import resources
from live2d.utils.lipsync import WavHandler

live2d.enableLog(True)
live2d.setLogLevel(live2d.Live2DLogLevels.LV_DEBUG)


def main():
    pygame.init()
    pygame.mixer.init()
    live2d.init()

    display = (500, 600)
    pygame.display.set_mode(display, DOUBLEBUF | OPENGL)
    pygame.display.set_caption("pygame window")

    if live2d.LIVE2D_VERSION == 3:
        live2d.glInit()

    model = live2d.LAppModel()


    if live2d.LIVE2D_VERSION == 3:
        model.LoadModelJson(
            # os.path.join(resources.RESOURCES_DIRECTORY, "v3/liveroid/liveroiD_A-Y01/liveroiD_A-Y01.model3.json")
            # os.path.join(resources.RESOURCES_DIRECTORY, "v3/Mao/Mao.model3.json")
            # os.path.join(resources.RESOURCES_DIRECTORY, "v3/llny/llny.model3.json")
            # os.path.join(resources.RESOURCES_DIRECTORY, "v3/nn/nn.model3.json")
            # os.path.join(resources.RESOURCES_DIRECTORY, "v3/magic/magic.model3.json")
            os.path.join(resources.RESOURCES_DIRECTORY, "v3/Haru/Haru.model3.json")
            # os.path.join(resources.RESOURCES_DIRECTORY, "v3/Hiyori/Hiyori.model3.json")
            # os.path.join(resources.RESOURCES_DIRECTORY, "v3/小九/小九皮套（红）/小九.model3.json")
            # os.path.join(resources.RESOURCES_DIRECTORY, "v3/金发大小姐/金发大小姐.model3.json")
        , maskBufferCount = 100)
    else:
        model.LoadModelJson(
            # os.path.join(resources.RESOURCES_DIRECTORY, "v2/托尔/model0.json")
            os.path.join(resources.RESOURCES_DIRECTORY, "v2/haru/haru.model.json")
        )


    model.Resize(*display)

    running = True

    dx: float = 0.0
    dy: float = 0.0
    scale: float = 1.0

    # 关闭自动眨眼
    model.SetAutoBlinkEnable(False)
    # 关闭自动呼吸
    model.SetAutoBreathEnable(False)

    wavHandler = WavHandler()
    lipSyncN = 3

    audioPlayed = False

    def on_start_motion_callback(group: str, no: int):
        log.Info("start motion: [%s_%d]" % (group, no))
        # play your voice here
        # audioPath = os.path.join(resources.CURRENT_DIRECTORY, "path to wav file")
        # pygame.mixer.music.load(audioPath)
        # pygame.mixer.music.play()
        # log.Info("start lipSync")
        # wavHandler.Start(audioPath)

    def on_finish_motion_callback():
        log.Info("motion finished")

    # 获取全部可用参数
    for i in range(model.GetParameterCount()):
        param = model.GetParameter(i)
        log.Debug(
            param.id, param.type, param.value, param.max, param.min, param.default
        )

    # 设置 part 透明度
    # log.Debug(f"Part Count: {model.GetPartCount()}")
    partIds = model.GetPartIds()
    # print(len(partIds))
    # log.Debug(f"Part Ids: {partIds}")
    # log.Debug(f"Part Id for index 2: {model.GetPartId(2)}")
    # model.SetPartOpacity(partIds.index("PartHairBack"), 0.5)

    currentTopClickedPartId = None

    def getHitFeedback(x, y):
        t = time.time()
        hitPartIds = model.HitPart(x, y, False)
        print(f"hit part cost: {time.time() - t}s")
        print(f"hit parts: {hitPartIds}")
        if currentTopClickedPartId is not None:
            pidx = partIds.index(currentTopClickedPartId)
            model.SetPartOpacity(pidx, 1)
            # model.SetPartScreenColor(pidx, 0.0, 0.0, 0.0, 1.0)
            model.SetPartMultiplyColor(pidx, 1.0, 1.0, 1., 1)
            # print("Part Screen Color:", model.GetPartScreenColor(pidx))
            print("Part Multiply Color:", model.GetPartMultiplyColor(pidx))
        if len(hitPartIds) > 0:
            ret = hitPartIds[0]
            return ret

    fc = None
    sc = None
    model.StartRandomMotion("TapBody", 300, sc, fc)

    radius_per_frame = math.pi * 10 / 1000 * 0.5
    deg_max = 5
    progress = 0
    deg = math.sin(progress) * deg_max 

    print("canvas size:", model.GetCanvasSize())
    print("canvas size in pixels:", model.GetCanvasSizePixel())
    print("pixels per unit:", model.GetPixelsPerUnit())

    while True:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
                break
            if event.type == pygame.MOUSEBUTTONDOWN:
                x, y = pygame.mouse.get_pos()
                # currentTopClickedPartId = getHitFeedback(x, y)
                # log.Info(f"Clicked Part: {currentTopClickedPartId}")
                # model.StartRandomMotion(group="TapBody", onFinishMotionHandler=lambda : print("motion finished"), onStartMotionHandler=lambda group, no: print(f"started motion: {group} {no}"))
                model.SetRandomExpression()
                model.StartRandomMotion(priority=3, onFinishMotionHandler=on_finish_motion_callback)

            if event.type == pygame.KEYDOWN:
                if event.key == pygame.K_LEFT:
                    dx -= 0.1
                elif event.key == pygame.K_RIGHT:
                    dx += 0.1

                elif event.key == pygame.K_UP:
                    dy += 0.1

                elif event.key == pygame.K_DOWN:
                    dy -= 0.1

                elif event.key == pygame.K_i:
                    scale += 0.1

                elif event.key == pygame.K_u:
                    scale -= 0.1
                
                elif event.key == pygame.K_r:
                    model.StopAllMotions()
                    model.ResetPose()
                
                elif event.key == pygame.K_e:
                    model.ResetExpression()

            if event.type == pygame.MOUSEMOTION:
                # 实现拖拽
                model.Drag(*pygame.mouse.get_pos())
                currentTopClickedPartId = getHitFeedback(*pygame.mouse.get_pos())

        if not running:
            break

        progress += radius_per_frame
        deg = math.sin(progress) * deg_max
        model.Rotate(deg)

        model.Update()

        if currentTopClickedPartId is not None:
            pidx = partIds.index(currentTopClickedPartId)
            model.SetPartOpacity(pidx, 0.5)
            # 在此以 255 为最大灰度级
            # 原色和屏幕色取反并相乘，再取反
            # 以红色通道为例：r = 255 - (255 - 原色.r) * (255 - screenColor.r) / 255
            # 通道数值越大，该通道颜色对最终结果的贡献越大，下面的调用即为突出蓝色的效果
            # model.SetPartScreenColor(pidx, .0, 0., 1.0, 1)

            # r = multiplyColor.r * 原色.r / 255
            # 下面即为仅保留蓝色通道的结果
            model.SetPartMultiplyColor(pidx, .0, .0, 1., .9)

        if wavHandler.Update():
            # 利用 wav 响度更新 嘴部张合
            model.SetParameterValue(
                StandardParams.ParamMouthOpenY, wavHandler.GetRms() * lipSyncN
            )

        if not audioPlayed:
            # 播放一个不存在的动作
            model.StartMotion(
                "",
                0,
                live2d.MotionPriority.FORCE,
                on_start_motion_callback,
                on_finish_motion_callback,
            )
            audioPlayed = True

        # 一般通过设置 param 去除水印
        # model.SetParameterValue("Param14", 1, 1)

        model.SetOffset(dx, dy)
        model.SetScale(scale)
        live2d.clearBuffer(1.0, 0.0, 0.0, 0.0)
        model.Draw()
        pygame.display.flip()
        pygame.time.wait(10)

    live2d.dispose()

    pygame.quit()
    quit()


if __name__ == "__main__":
    currentTopClickedPartId = None
    main()
