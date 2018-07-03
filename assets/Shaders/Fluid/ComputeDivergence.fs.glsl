#version 150

uniform sampler2DRect uVelocityBuffer;
uniform sampler2DRect uObstacleBuffer;

uniform float uHalfInverseCellSize;

out float FinalColor;
in vec2 uv;

void main()
{
    vec2 vN = texture ( uVelocityBuffer, uv + vec2 (  0.0,  1.0 ) ).rg;
    vec2 vS = texture ( uVelocityBuffer, uv + vec2 (  0.0, -1.0 ) ).rg;
    vec2 vE = texture ( uVelocityBuffer, uv + vec2 (  1.0,  0.0 ) ).rg;
    vec2 vW = texture ( uVelocityBuffer, uv + vec2 ( -1.0,  0.0 ) ).rg;
   
    vec3 oN = texture ( uObstacleBuffer, uv + vec2 (  0.0,  1.0 ) ).rgb;
    vec3 oS = texture ( uObstacleBuffer, uv + vec2 (  0.0, -1.0 ) ).rgb;
    vec3 oE = texture ( uObstacleBuffer, uv + vec2 (  1.0,  0.0 ) ).rgb;
    vec3 oW = texture ( uObstacleBuffer, uv + vec2 ( -1.0,  0.0 ) ).rgb;
   
    if ( oN.x > 0.1 ) vN = oN.yz;
    if ( oS.x > 0.1 ) vS = oS.yz;
    if ( oE.x > 0.1 ) vE = oE.yz;
    if ( oW.x > 0.1 ) vW = oW.yz;
   
    FinalColor = uHalfInverseCellSize * ( vE.x - vW.x + vN.y - vS.y );
}