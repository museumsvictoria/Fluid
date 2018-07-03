#version 150

uniform sampler2DRect uVelocityBuffer;
uniform sampler2DRect uSourceBuffer;
uniform sampler2DRect uObstacleBuffer;

uniform float uTimeStep;
uniform float uDissipation;

out vec4 FinalColor;
in vec2 uv;

void main()
{
    float solid = texture ( uObstacleBuffer, uv ).r;
    if ( solid > 0.1 ) 
    {
        FinalColor = vec4 ( 0.0 );
        return;
    }
    
    vec2 u = texture ( uVelocityBuffer, uv ).rg;
    vec2 coord =  uv - uTimeStep * u;
    
    FinalColor = texture ( uSourceBuffer, coord ) * uDissipation;
}