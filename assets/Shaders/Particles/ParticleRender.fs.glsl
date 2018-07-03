#version 150

uniform sampler2D uParticle;
uniform sampler2DRect uDensity;
uniform float uAlphaMultiplier = 0.8;
uniform float uAlphaMin = 0.0;
uniform float uGlobalAlpha;
uniform float uDensityContribution;

in float Life;
in vec2 UV;
in vec4 Velocity;

out vec4 FinalColor;

void main ( )
{
	float a = uAlphaMin + ( length ( Velocity.xyz ) * uAlphaMultiplier );
    a *= uGlobalAlpha;

    if ( a < 0.05 )
    {
        discard;
        return;
    }
    
    vec3 color = texture ( uDensity, UV ).rgb * vec3(0.05);
    float pA = texture ( uParticle, gl_PointCoord.xy ).a * a;
    
    FinalColor = vec4 ( color, pA );
}