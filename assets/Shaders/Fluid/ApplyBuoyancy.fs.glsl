#version 150
#define MAX_ATTRACTORS 4

uniform sampler2DRect uVelocityBuffer;
uniform sampler2DRect uTemperatureBuffer;
uniform sampler2DRect uDensityBuffer;

uniform float uAmbientTemperature = 0.0;
uniform float uTimeStep;
uniform float uSigma;
uniform float uKappa;

uniform vec2  uGravity;
uniform vec2  uAttract;

struct Attractor
{
    bool  Enabled;
    float Radius;
    float Force;
    vec2  Position;
};

uniform Attractor uAttractors[MAX_ATTRACTORS];

out vec2 FinalColor;
in vec2 uv;

void main()
{
	float T = texture ( uTemperatureBuffer, uv ).r;
    vec2 V = texture ( uVelocityBuffer, uv ).rg;
    
    FinalColor = V;
   
    if ( T > uAmbientTemperature ) 
    {
        float D = texture ( uDensityBuffer, uv ).r;
        FinalColor += ( uTimeStep * ( T - uAmbientTemperature ) * uSigma - D * uKappa ) * uGravity;

        for ( int i = 0; i < MAX_ATTRACTORS; i++ )
        {
            if ( !uAttractors[i].Enabled ) continue;
            vec2 aDist = uv - uAttractors[i].Position;
            float len = length(aDist);

            if ( len > 0.0f && len <= uAttractors[i].Radius )
            {
                vec2 attract = vec2( aDist.x / len, aDist.y / len ) * uAttractors[i].Force;
                FinalColor -= ( uTimeStep * ( T - uAmbientTemperature ) * uSigma - D * uKappa ) * attract;
            }
        }
    }
}