import live2d
import glfw
import os
import OpenGL.GL as GL
import random

RESOURCES = os.path.abspath(
    os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "Resources")
)

LIVE2D_VERSION = 3

def main():
    glfw.init()
    live2d.init()

    frame = (1024, 720)

    model = live2d.Model()
    if LIVE2D_VERSION == 3:
        model.LoadModelJson(os.path.join(RESOURCES, "v3", "llny/llny.model3.json"), False)
    else:
        model.LoadModelJson(os.path.join(RESOURCES, "v2", "kasumi2/kasumi2.model.json"), False)

    window = glfw.create_window(*frame, "test exp fade", None, None)
    glfw.make_context_current(window)
    live2d.glInit()
    model.CreateRenderer(2)


    def on_resize(window, w, h):
        GL.glViewport(0, 0, w, h)
        model.Resize(w, h)
    on_resize(window, *frame)
    glfw.set_window_size_callback(window, on_resize)

    expressionIds = model.GetExpressions()
    def on_mouse_left_button(window, button, action, mods):
        if button == glfw.MOUSE_BUTTON_LEFT and action == glfw.PRESS:
            model.SetExpression(random.choice(expressionIds), 5000)
        if button == glfw.MOUSE_BUTTON_RIGHT and action == glfw.PRESS:
            model.AddExpression(random.choice(expressionIds))
    glfw.set_mouse_button_callback(window, on_mouse_left_button)

    while not glfw.window_should_close(window):
        glfw.poll_events()
        live2d.clearBuffer()
        model.Update()
        model.Draw()

        glfw.swap_buffers(window)

    live2d.glRelease()

    glfw.destroy_window(window)
    glfw.terminate()
    live2d.dispose()


if __name__ == "__main__":
    main()