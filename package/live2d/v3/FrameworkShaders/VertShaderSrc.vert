#version 120

attribute vec4 a_position; //v.vertex
attribute vec2 a_texCoord; //v.texcoord
varying vec2 v_texCoord; //v2f.texcoord
uniform mat4 u_matrix;

void main()
{
    gl_Position = u_matrix * a_position;
    v_texCoord = a_texCoord;
    v_texCoord.y = 1.0 - v_texCoord.y;
}
