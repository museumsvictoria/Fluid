#version 150

uniform mat4 ciModelViewProjection;
in vec2 ciTexCoord0;
in vec4 ciPosition;

out vec2 UV;

void main ( )
{
    gl_Position = ciModelViewProjection * vec4 ( ciPosition.xy, 0, 1 );
    UV = ciTexCoord0;
}
