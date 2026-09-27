import os
import glfw
import resources
import live2d


def main():
    if not glfw.init():
        raise RuntimeError("glfw.init failed")
    display = (800, 500)
    window = glfw.create_window(*display, "glfw - multi models", None, None)
    if not window:
        glfw.terminate()
        return
    glfw.make_context_current(window)

    live2d.init()
    live2d.glInit()

    model_v2 = live2d.Model()
    model_v2_2 = live2d.Model()
    model_v3 = live2d.Model()
    model_v3_2 = live2d.Model()

    model_v3.LoadModelJson(
        os.path.join(resources.RESOURCES_DIRECTORY, "v3/llny/llny.model3.json")
    )
    model_v3_2.LoadModelJson(
        os.path.join(resources.RESOURCES_DIRECTORY, "v3/Haru/Haru.model3.json")
    )
    model_v2.LoadModelJson(
        os.path.join(resources.RESOURCES_DIRECTORY, "v2/托尔/model0.json")
    )
    model_v2_2.LoadModelJson(
        os.path.join(resources.RESOURCES_DIRECTORY, "v2/kasumi2/kasumi2.model.json")
    )

    model_v3.Resize(*display)
    model_v3_2.Resize(*display)
    model_v2.Resize(*display)
    model_v2_2.Resize(*display)

    model_v3.SetOffset(-0.6, 0.0)
    model_v3_2.SetOffset(-0.2, 0.0)
    model_v2.SetOffset(0.2, 0.0)
    model_v2_2.SetOffset(0.6, 0.3)
    model_v3.SetScale(0.7)
    model_v3_2.SetScale(0.7)
    model_v2.SetScale(0.7)
    model_v2_2.SetScale(0.7)

    def on_cursor_pos(window, x, y):
        model_v3.Drag(x, y)
        model_v3_2.Drag(x, y)
        model_v2.Drag(x, y)
        model_v2_2.Drag(x, y)
    glfw.set_cursor_pos_callback(window, on_cursor_pos)

    glfw.swap_interval(1)

    while not glfw.window_should_close(window):
        glfw.poll_events()

        live2d.clearBuffer()

        model_v3_2.Update()
        model_v3_2.Draw()

        model_v3.Update()
        model_v3.Draw()

        model_v2.Update()
        model_v2.Draw()

        model_v2_2.Update()
        model_v2_2.Draw()

        glfw.swap_buffers(window)

    live2d.dispose()
    glfw.terminate()


if __name__ == "__main__":
    main()
