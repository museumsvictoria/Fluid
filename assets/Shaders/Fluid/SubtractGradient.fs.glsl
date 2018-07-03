#version 150

uniform sampler2DRect uVelocityBuffer;
uniform sampler2DRect uPressureBuffer;
uniform sampler2DRect uObstacleBuffer;

uniform float uGradientScale;

out vec2 FinalColor;
in vec2 uv;

void main()
{
    vec3 oC = texture ( uObstacleBuffer, uv ).rgb;

    if ( oC.x > 0.1 ) 
    {
        FinalColor = oC.yz;
        return;
    }
    
    float pN = texture ( uPressureBuffer, uv + vec2 (  0.0,  1.0 ) ).r;
    float pS = texture ( uPressureBuffer, uv + vec2 (  0.0, -1.0 ) ).r;
    float pE = texture ( uPressureBuffer, uv + vec2 (  1.0,  0.0 ) ).r;
    float pW = texture ( uPressureBuffer, uv + vec2 ( -1.0,  0.0 ) ).r;
    float pC = texture ( uPressureBuffer, uv ).r;
    
    vec3 oN = texture ( uObstacleBuffer, uv + vec2 (  0.0,  1.0 ) ).rgb;
    vec3 oS = texture ( uObstacleBuffer, uv + vec2 (  0.0, -1.0 ) ).rgb;
    vec3 oE = texture ( uObstacleBuffer, uv + vec2 (  1.0,  0.0 ) ).rgb;
    vec3 oW = texture ( uObstacleBuffer, uv + vec2 ( -1.0,  0.0 ) ).rgb;
    
    vec2 obstV = vec2 ( 0.0, 0.0 );
    vec2 vMask = vec2 ( 1.0, 1.0 );
    
    if ( oN.x > 0.1 ) { pN = pC; obstV.y = oN.z; vMask.y = 0.0; }
    if ( oS.x > 0.1 ) { pS = pC; obstV.y = oS.z; vMask.y = 0.0; }
    if ( oE.x > 0.1 ) { pE = pC; obstV.x = oE.y; vMask.x = 0.0; }
    if ( oW.x > 0.1 ) { pW = pC; obstV.x = oW.y; vMask.x = 0.0; }
    
    vec2 oldV = texture( uVelocityBuffer, uv ).rg;
    vec2 grad = vec2 ( pE - pW, pN - pS ) * uGradientScale;
    vec2 newV = oldV - grad;
    
    FinalColor = (vMask * newV) + obstV;
}