import live2d.v2 as v2
import live2d.v3 as v3
import live2d.v2cpp as v2cpp


import os
import glfw

RESOURCES_DIRECTORY = os.path.split(__file__)[0] + "/../Resources"


def main():
    if not glfw.init():
        raise RuntimeError("glfw.init failed")
    display = (800, 500)
    window = glfw.create_window(*display, "glfw - multi models", None, None)
    if not window:
        glfw.terminate()
        return
    glfw.make_context_current(window)

    v3.init()
    v2.init()
    v2cpp.init()
    v3.glInit()
    v2cpp.glInit()
    v2.glInit()

    model_v2 = v2.LAppModel()
    model_v2cpp = v2cpp.LAppModel()
    model_v3 = v3.LAppModel()
    model_v3_2 = v3.LAppModel()

    model_v3.LoadModelJson(
        os.path.join(RESOURCES_DIRECTORY, "v3/llny/llny.model3.json")
    )
    model_v3_2.LoadModelJson(
        os.path.join(RESOURCES_DIRECTORY, "v3/Haru/Haru.model3.json")
    )
    model_v2cpp.LoadModelJson(
        os.path.join(RESOURCES_DIRECTORY, "v2/托尔/model0.json")
    )
    model_v2.LoadModelJson(
        os.path.join(RESOURCES_DIRECTORY, "v2/kasumi2/kasumi2.model.json")
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

    current_model_index = 0
    models = [model_v3, model_v2, model_v3_2, model_v2cpp]
    def on_mouse_left_button(window, button, action, modes):
        if button != glfw.MOUSE_BUTTON_LEFT or action != glfw.PRESS:
            return
        nonlocal current_model_index, models
        current_model_index = (current_model_index + 1) % len(models)
    glfw.set_mouse_button_callback(window, on_mouse_left_button)

    glfw.swap_interval(1)

    while not glfw.window_should_close(window):
        glfw.poll_events()

        v3.clearBuffer()

        models[current_model_index].Update()
        models[current_model_index].Draw()

        glfw.swap_buffers(window)

    v3.dispose()
    v2.dispose()
    v2cpp.dispose()
    glfw.terminate()


if __name__ == "__main__":
    main()
