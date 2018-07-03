#version 150

uniform sampler2DRect uPressureBuffer;
uniform sampler2DRect uDivergenceBuffer;
uniform sampler2DRect uObstacleBuffer;

uniform float uAlpha;
uniform float uInverseBeta;

out vec4 FinalColor;
in vec2 uv;

void main() 
{
    vec4 pN = texture ( uPressureBuffer, uv + vec2 (  0.0,  1.0 ) );
    vec4 pS = texture ( uPressureBuffer, uv + vec2 (  0.0, -1.0 ) );
    vec4 pE = texture ( uPressureBuffer, uv + vec2 (  1.0,  0.0 ) );
    vec4 pW = texture ( uPressureBuffer, uv + vec2 ( -1.0,  0.0 ) );
    vec4 pC = texture ( uPressureBuffer, uv );
    
    vec3 oN = texture ( uObstacleBuffer, uv + vec2 (  0.0,  1.0 ) ).rgb;
    vec3 oS = texture ( uObstacleBuffer, uv + vec2 (  0.0, -1.0 ) ).rgb;
    vec3 oE = texture ( uObstacleBuffer, uv + vec2 (  1.0,  0.0 ) ).rgb;
    vec3 oW = texture ( uObstacleBuffer, uv + vec2 ( -1.0,  0.0 ) ).rgb;
    
    if ( oN.x > 0.1 ) pN = pC;
    if ( oS.x > 0.1 ) pS = pC;
    if ( oE.x > 0.1 ) pE = pC;
    if ( oW.x > 0.1 ) pW = pC;
    
    vec4 bC = texture ( uDivergenceBuffer, uv );
    FinalColor = ( pW + pE + pS + pN + uAlpha * bC ) * uInverseBeta;
}