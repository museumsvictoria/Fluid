#version 150

uniform mat4 ciModelViewProjection;

in vec4 ciPosition;

uniform sampler2DRect uVelocity;
uniform sampler2DRect uDensity;
uniform float uColorWeight = 1.0;

uniform vec2 uSize;
uniform float uWeight = 0.4f;

out vec3 Color;

void main ( )
{
    vec2 offset = texture ( uVelocity, ciPosition.xy ).rg;
    Color = mix( vec3(1), texture ( uDensity, ciPosition.xy ).rgb * 0.05, uColorWeight );
    
    vec4 pos = ciPosition;
    pos.xy += ( offset * uWeight );

    gl_Position = ciModelViewProjection * pos;
}