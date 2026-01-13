import os
import pygame
import live2d.v3 as l2d_v3
import live2d.v2 as l2d_v2
import resources

import faulthandler
faulthandler.enable()

def main():
    pygame.init()
    l2d_v3.init()
    l2d_v2.init()

    display = (800, 500)
    pygame.display.set_mode(display, pygame.DOUBLEBUF | pygame.OPENGL)
    pygame.display.set_caption("pygame window")

    l2d_v3.glInit()
    l2d_v2.glInit()

    model_v2 = l2d_v2.LAppModel()
    model_v3 = l2d_v3.LAppModel()
    model_v3_2 = l2d_v3.LAppModel()

    model_v3.LoadModelJson(
        os.path.join(resources.RESOURCES_DIRECTORY, "v3/llny/llny.model3.json")
    )
    model_v3_2.LoadModelJson(
        os.path.join(resources.RESOURCES_DIRECTORY, "v3/Haru/Haru.model3.json")
    )
    model_v2.LoadModelJson(
        os.path.join(resources.RESOURCES_DIRECTORY, "v2/kasumi2/kasumi2.model.json")
    )

    model_v3.Resize(*display)
    model_v2.Resize(*display)
    model_v3_2.Resize(*display)

    model_v3.SetOffset(-0.5, 0.0)
    model_v2.SetOffset(0.5, 0.3)
    model_v2.SetScale(0.7)

    running = True
    while True:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
                break
            if event.type == pygame.MOUSEMOTION:
                x, y = pygame.mouse.get_pos()
                model_v3.Drag(x, y)
                model_v2.Drag(x, y)
                model_v3_2.Drag(x, y)

        if not running:
            break

        l2d_v3.clearBuffer()

        model_v3_2.Update()
        model_v3_2.Draw()

        model_v3.Update()
        model_v3.Draw()
        
        model_v2.Update()
        model_v2.Draw()

        pygame.display.flip()

    l2d_v3.dispose()
    l2d_v2.dispose()

    pygame.quit()


if __name__ == "__main__":
    main()
