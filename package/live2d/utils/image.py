from .opengl_functions import create_vao, create_program, create_texture

import OpenGL.GL as GL
import numpy as np


class Image:

    def __init__(self, imagePath: str):
        vertex_shader = """#version 330 core
        layout(location = 0) in vec2 a_position;
        layout(location = 1) in vec2 a_texCoord;
        out vec2 v_texCoord;
        void main() {
            gl_Position = vec4(a_position, 0.0, 1.0);
            v_texCoord = a_texCoord;
        }
        """
        frag_shader = """#version 330 core
        in vec2 v_texCoord;
        uniform sampler2D tex;
        uniform float opacity;
        void main() {
            vec4 col = texture(tex, v_texCoord);
            gl_FragColor = col;
        }
        """
        self.program = create_program(vertex_shader, frag_shader)
        self.texture = create_texture(imagePath)

        vertices = np.array([
            # 位置
            -1, 1,
            -1, -1,
            1, -1,
            -1, 1,
            1, -1,
            1, 1,
        ], dtype=np.float32)
        uvs = np.array([
            # 纹理坐标
            0, 1,
            0, 0,
            1, 0,
            0, 1,
            1, 0,
            1, 1
        ], dtype=np.float32)
        self.vao = create_vao(vertices, uvs)

    def Draw(self):
        GL.glBindVertexArray(self.vao)
        GL.glUseProgram(self.program)
        GL.glActiveTexture(GL.GL_TEXTURE0)
        GL.glBindTexture(GL.GL_TEXTURE_2D, self.texture)
        GL.glDrawArrays(GL.GL_TRIANGLES, 0, 6)
        GL.glBindVertexArray(0)
        GL.glUseProgram(0)
