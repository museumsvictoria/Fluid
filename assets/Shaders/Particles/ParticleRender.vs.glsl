#version 150

uniform mat4 ciModelViewProjection;
uniform ivec2 ciWindowSize;

in vec2 ciTexCoord0;

uniform sampler2D uPositionBuffer;
uniform sampler2D uVelocityBuffer;
uniform float uMaxParticleSize = 16.0;

out float Life;
out vec4 Velocity;
out vec2 UV;

void main ( )
{
    vec4 pos = texture ( uPositionBuffer, ciTexCoord0 );
    Velocity = texture ( uVelocityBuffer, ciTexCoord0 );

    Life = pos.z / pos.w;
    
    UV = pos.xy * 0.5f;

    gl_PointSize = (1.0 - Life) * uMaxParticleSize;
    gl_Position = ciModelViewProjection * vec4 ( pos.xy, 0, 1 );

}
