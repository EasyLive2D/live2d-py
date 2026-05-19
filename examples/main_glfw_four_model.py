import os
import glfw
import live2d.v3 as l2d_v3
import live2d.v2 as l2d_v2
import live2d.v2cpp as l2d_v2cpp
import resources


def main():
    if not glfw.init():
        raise RuntimeError("glfw.init failed")
    display = (800, 500)
    window = glfw.create_window(*display, "glfw - multi models", None, None)
    if not window:
        glfw.terminate()
        return
    glfw.make_context_current(window)

    l2d_v3.init()
    l2d_v2.init()
    l2d_v2cpp.init()
    l2d_v3.glInit()
    l2d_v2cpp.glInit()

    model_v2 = l2d_v2.LAppModel()
    model_v2cpp = l2d_v2cpp.LAppModel()
    model_v3 = l2d_v3.LAppModel()
    model_v3_2 = l2d_v3.LAppModel()

    model_v3.LoadModelJson(
        os.path.join(resources.RESOURCES_DIRECTORY, "v3/llny/llny.model3.json")
    )
    model_v3_2.LoadModelJson(
        os.path.join(resources.RESOURCES_DIRECTORY, "v3/Haru/Haru.model3.json")
    )
    model_v2cpp.LoadModelJson(
        os.path.join(resources.RESOURCES_DIRECTORY, "v2/托尔/model0.json")
    )
    model_v2.LoadModelJson(
        os.path.join(resources.RESOURCES_DIRECTORY, "v2/kasumi2/kasumi2.model.json")
    )

    model_v3.Resize(*display)
    model_v3_2.Resize(*display)
    model_v2cpp.Resize(*display)
    model_v2.Resize(*display)

    model_v3.SetOffset(-0.6, 0.0)
    model_v3_2.SetOffset(-0.2, 0.0)
    model_v2cpp.SetOffset(0.2, 0.0)
    model_v2.SetOffset(0.6, 0.3)
    model_v3.SetScale(0.7)
    model_v3_2.SetScale(0.7)
    model_v2cpp.SetScale(0.7)
    model_v2.SetScale(0.7)

    def on_cursor_pos(window, x, y):
        model_v3.Drag(x, y)
        model_v3_2.Drag(x, y)
        model_v2cpp.Drag(x, y)
        model_v2.Drag(x, y)
    glfw.set_cursor_pos_callback(window, on_cursor_pos)

    glfw.swap_interval(1)

    while not glfw.window_should_close(window):
        glfw.poll_events()

        l2d_v3.clearBuffer()

        model_v3_2.Update()
        model_v3_2.Draw()

        model_v3.Update()
        model_v3.Draw()

        model_v2cpp.Update()
        model_v2cpp.Draw()

        model_v2.Update()
        model_v2.Draw()

        glfw.swap_buffers(window)

    l2d_v3.dispose()
    l2d_v2.dispose()
    l2d_v2cpp.dispose()
    glfw.terminate()


if __name__ == "__main__":
    main()
