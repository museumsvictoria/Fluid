#version 150

uniform sampler2DRect uSourceBuffer;
uniform sampler2DRect uTexture;

uniform float uWeight;
uniform int uIsVelocity;

out vec4 FinalColor;
in vec2 uv;

void main()
{
    vec4 prevFrame = texture ( uSourceBuffer, uv );
    vec4 newFrame = texture ( uTexture, uv );
    
    if (uIsVelocity != 0 ) 
    {
        newFrame -= 0.5;
        newFrame *= 2.0;
        newFrame.b = 0.5;
    }
  
    FinalColor = prevFrame + newFrame * uWeight;
}