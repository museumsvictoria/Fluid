#version 150

uniform vec2    uPoint;
uniform float   uRadius;
uniform vec3    uValue;

out vec4 FinalColor;
in vec2 uv;

void main()
{
    float d = distance ( uPoint, uv );
    
    if ( d < uRadius ) 
    {
        float a = ( uRadius - d ) * 0.5;
        a = min(a, 1.0);
        FinalColor = vec4 ( uValue, a );
    } else 
    {
        FinalColor = vec4(0);
    }
}