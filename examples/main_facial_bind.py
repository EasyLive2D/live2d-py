# 简易面捕示例

import os
import threading as td

import pygame
import live2d.v3 as live2d
# import live2d.v2 as live2d
import resources
from mediapipe_capture.facial_params import Params
if live2d.LIVE2D_VERSION == 3:
    from live2d.v3.params import StandardParams
elif live2d.LIVE2D_VERSION == 2:
    from live2d.v2.params import StandardParams
from mediapipe_capture.new_capture_task import capture_task


live2d.enableLog(False)


def s_call(group, no):
    print(group, no)


def f_call():
    print("end")


def main():
    pygame.init()
    live2d.init()

    display = (450, 700)
    pygame.display.set_mode(display, pygame.DOUBLEBUF | pygame.OPENGL)

    live2d.glInit()

    model = live2d.LAppModel()

    if live2d.LIVE2D_VERSION == 3:
        model.LoadModelJson(os.path.join(resources.RESOURCES_DIRECTORY, 
                                         "v3/llny/llny.model3.json"
                                        # "v3/whitecat/sdwhite cat free.model3.json"
                                        # "v3/小九/小九皮套（紫）/小九.model3.json"
                                        # "v3/魅魔喵/meimo1.model3.json"
                                        # "v3/monv/monv.model3.json"
                                         ))
    elif live2d.LIVE2D_VERSION == 2:
        model.LoadModelJson(os.path.join(resources.RESOURCES_DIRECTORY, "v2/托尔/model0.json"))
    model.Resize(*display)

    running = True

    params = Params()
    td.Thread(None, capture_task, "Capture Task", (params,0), daemon=True).start()

    model.SetAutoBreathEnable(False)
    model.SetAutoBlinkEnable(False)

    while True:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
                break
            if event.type == pygame.MOUSEBUTTONDOWN:
                print("set random expression")
                model.SetRandomExpression()

        if not running:
            break

        if params:
            # 较大程度的解決抖动问题，Params类中的smooth_factor控制平滑度
            params.update_params(params)
            # 面捕贴合程度取决于面部特征识别和参数计算算法
            # eye
            model.SetParameterValue(StandardParams.ParamEyeLOpen, params.EyeLOpen, 1)
            model.SetParameterValue(StandardParams.ParamEyeROpen, params.EyeROpen, 1)
            # mouth
            model.SetParameterValue(StandardParams.ParamMouthOpenY, params.MouthOpenY, 1)
            model.SetParameterValue(StandardParams.ParamMouthForm, params.MouthForm, 1)
            # head pose
            model.SetParameterValue(StandardParams.ParamAngleX, params.AngleX, 1)
            model.SetParameterValue(StandardParams.ParamAngleY, params.AngleY, 1)
            model.SetParameterValue(StandardParams.ParamAngleZ, params.AngleZ, 1)
            # iris
            model.SetParameterValue(StandardParams.ParamEyeBallX, params.EyeBallX, 1)


        # 去除水印
        model.SetParameterValue("Param14", 1, 1)

        live2d.clearBuffer()
        model.Update()
        model.Draw()
        pygame.display.flip()
        pygame.time.wait(int(1000 / 60))

    live2d.dispose()
    pygame.quit()
    quit()


if __name__ == "__main__":
    main()
