#version 150

uniform sampler2D uMatCap;
uniform sampler2DRect uDensity;
uniform sampler2DRect uVelocity;

uniform vec2 uSceneScale;

uniform float uPerturbation = 0.3;
uniform float uPerturbationExponent = 1.0f;
uniform float uMetalness = 1.0;
uniform vec4 uDensityColor;

in vec2 UV;

out vec4 FinalColor;


void main ( )
{
    vec2 matCapUV = vec2(UV.x, 1.0-UV.y);
    vec4 density = texture(uDensity, UV * uSceneScale);
    
    float d = length(density.rgb) / sqrt(3.0);

    vec2 deriv = density.xy * 2.0 - 1.0;
    deriv *= uPerturbation;

    FinalColor = texture ( uMatCap, matCapUV + deriv ) * clamp ( pow ( d, uPerturbationExponent ), 0, 1 );
    FinalColor.rgb = mix ( density.rgb * uDensityColor.rgb, FinalColor.rgb, uMetalness );
    FinalColor.a = density.a * uDensityColor.a;
}