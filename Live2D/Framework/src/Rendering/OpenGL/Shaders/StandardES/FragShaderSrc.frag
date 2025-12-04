#version 100

precision highp float;

varying vec2 v_texCoord; //v2f.texcoord
uniform sampler2D s_texture0; //_MainTex
uniform vec4 u_baseColor; //v2f.color
uniform vec4 u_multiplyColor;
uniform vec4 u_screenColor;

void main()
{
    vec4 texColor = texture2D(s_texture0 , v_texCoord);
    texColor.rgb = texColor.rgb * u_multiplyColor.rgb;
    texColor.rgb = texColor.rgb + u_screenColor.rgb - (texColor.rgb * u_screenColor.rgb);
    vec4 color = texColor * u_baseColor;
    gl_FragColor = vec4(color.rgb * color.a,  color.a);
}
