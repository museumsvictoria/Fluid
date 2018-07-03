#version 150

uniform float uAlpha;
in vec3 Color;
out vec4 FinalColor;

void main ( )
{
    FinalColor = vec4(Color, uAlpha);
}