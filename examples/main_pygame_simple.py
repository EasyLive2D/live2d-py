import os
import pygame
# import live2d.v3 as live2d
import live2d.v2 as live2d
if live2d.LIVE2D_VERSION == 3:
    from live2d.v3 import StandardParams
else:
    from live2d.v2 import StandardParams
import resources


def main():
    pygame.init()
    live2d.init()

    display = (300, 400)
    pygame.display.set_mode(display, pygame.DOUBLEBUF | pygame.OPENGL)
    pygame.display.set_caption("pygame window")

    live2d.glInit()

    model = live2d.LAppModel()

    if live2d.LIVE2D_VERSION == 3:
        model.LoadModelJson(
            os.path.join(resources.RESOURCES_DIRECTORY, "v3/Haru/Haru.model3.json")
        )
    else:
        model.LoadModelJson(
            os.path.join(resources.RESOURCES_DIRECTORY, 
                         "v2/kasumi2/kasumi2.model.json"
                        # "v2/haru/haru.model.json"
                        # "v2/托尔/model0.json"
                        # "v2/kana/Kobayaxi.model.json"
                         # "v2/shizuku/shizuku.model.json"
                         )
        )

    model.Resize(*display)
    model.SetAutoBlinkEnable(False)
    model.SetParameterValue(StandardParams.ParamEyeLOpen, 0)

    running = True
    while True:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
                break
            elif event.type == pygame.MOUSEMOTION:
                model.Drag(*pygame.mouse.get_pos())
            elif event.type == pygame.MOUSEBUTTONUP:
                model.SetRandomExpression()
                model.StartRandomMotion()
        
        if not running:
            break

        live2d.clearBuffer()
        model.Update()

        model.Draw()

        pygame.display.flip()

    live2d.dispose()

    pygame.quit()

if __name__ == "__main__":
    main()
