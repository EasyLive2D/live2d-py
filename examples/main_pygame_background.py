import os
import pygame
import live2d.v3 as live2d
# import live2d.v2 as live2d
import resources

from live2d.utils.image import Image

def main():
    pygame.init()
    live2d.init()

    display = (800, 600)
    pygame.display.set_mode(display, pygame.DOUBLEBUF | pygame.OPENGL)
    pygame.display.set_caption("pygame window")

    live2d.glInit()

    model = live2d.LAppModel()
    background = Image(
        os.path.join(resources.RESOURCES_DIRECTORY, "RING.png")
    )

    if live2d.LIVE2D_VERSION == 3:
        model.LoadModelJson(
            os.path.join(resources.RESOURCES_DIRECTORY, "v3/Haru/Haru.model3.json")
        )
    else:
        model.LoadModelJson(
            os.path.join(resources.RESOURCES_DIRECTORY, "v2/kasumi2/kasumi2.model.json")
        )

    model.Resize(*display)

    running = True
    while True:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
                break
            if event.type == pygame.MOUSEMOTION:
                model.Drag(*pygame.mouse.get_pos())
        
        if not running:
            break

        live2d.clearBuffer()
        background.Draw()
        model.Update()
        model.Draw()

        pygame.display.flip()

    live2d.dispose()

    pygame.quit()

if __name__ == "__main__":
    main()
