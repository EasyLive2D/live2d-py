import live2d.v2 as v2
import live2d.v3 as v3
import live2d.v2cpp as v2cpp

import os
import glfw
import random

RESOURCES_DIRECTORY = os.path.split(__file__)[0] + "/../Resources"


v2.setLogLevel(v2.Live2DLogLevels.LV_ERROR)
v3.setLogLevel(v3.Live2DLogLevels.LV_ERROR)
v2cpp.setLogLevel(v2cpp.Live2DLogLevels.LV_ERROR)


from OpenGL.GL import *
import traceback

def check_gl_error(tag=""):
    err = glGetError()
    while err != GL_NO_ERROR:
        name = {
            GL_INVALID_ENUM: "GL_INVALID_ENUM",
            GL_INVALID_VALUE: "GL_INVALID_VALUE",
            GL_INVALID_OPERATION: "GL_INVALID_OPERATION",
            GL_OUT_OF_MEMORY: "GL_OUT_OF_MEMORY",
            GL_INVALID_FRAMEBUFFER_OPERATION: "GL_INVALID_FRAMEBUFFER_OPERATION",
        }.get(err, f"UNKNOWN({err})")
        print(f"[GL ERROR] {tag}: {name}")
        err = glGetError()
        raise RuntimeError


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

    current_model_index = 1
    current_model = None
    current_frame = 0
    def on_cursor_pos(window, x, y):
        if current_model:
            current_model.Drag(x, y)

    glfw.set_cursor_pos_callback(window, on_cursor_pos)


    def on_mouse_left_button(window, button, action, modes):
        if button != glfw.MOUSE_BUTTON_LEFT or action != glfw.PRESS:
            return
        nonlocal current_model_index, current_model, current_frame
        check_gl_error("0")
        current_frame = 0
        if current_model:
            del current_model
        current_model_index = random.randint(0, 3)
        check_gl_error("1")
        match current_model_index:
            case 0:
                print("v3 => llny/llny.model3.json")
                current_model = v3.LAppModel()
                current_model.LoadModelJson(
                    os.path.join(RESOURCES_DIRECTORY, "v3/llny/llny.model3.json")
                )
            case 2:
                print("v3 => Haru/Haru.model3.json")
                current_model = v3.LAppModel()
                current_model.LoadModelJson(
                    os.path.join(RESOURCES_DIRECTORY, "v3/Haru/Haru.model3.json")
                )
            case 1:
                print("v2cpp => 托尔/model0.json")
                current_model = v2cpp.LAppModel()
                current_model.LoadModelJson(
                    os.path.join(RESOURCES_DIRECTORY, "v2/托尔/model0.json")
                )
            case 3:
                print("v2 => kasumi2/kasumi2.model.json")
                current_model = v2.LAppModel()
                current_model.LoadModelJson(
                    os.path.join(RESOURCES_DIRECTORY, "v2/kasumi2/kasumi2.model.json")
                )
        check_gl_error("2")
        current_model.Resize(*display)
        check_gl_error("3")

    glfw.set_mouse_button_callback(window, on_mouse_left_button)

    def on_resize(window, w, h):
        current_model.Resize(w, h)
        glViewport(0, 0, w, h)
        nonlocal display
        display = (w, h)
    glfw.set_window_size_callback(window, on_resize)

    glfw.swap_interval(1)


    while not glfw.window_should_close(window):
        glfw.poll_events()

        v3.clearBuffer()

        if current_model:
            current_frame += 1
            print("current frame:", current_frame)
        if current_model:
            current_model.Update()
            current_model.Draw()
        check_gl_error("draw")

        glfw.swap_buffers(window)

    v3.dispose()
    v2.dispose()
    v2cpp.dispose()
    glfw.terminate()


if __name__ == "__main__":
    main()
