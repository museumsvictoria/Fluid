#version 150

in vec2 UV;

out vec4 NewPosition;
out vec4 NewVelocity;

uniform sampler2D uPositionBuffer;
uniform sampler2D uVelocityBuffer;
uniform sampler2D uOriginalPositions;
uniform sampler2D uOriginalVelocities;
uniform sampler2DRect uVelocityField;

uniform vec2 uVelocityFieldSize;
uniform float uVelocityMultiplier = 0.004;
uniform float uVelocityDamping = 0.96;

void main ( )
{
	vec4 pos = texture ( uPositionBuffer, UV );
    vec4 vel = texture ( uVelocityBuffer, UV );

	vec2 velPos = pos.xy * 0.5f; // This is the "scale" of the fluid, i.e uVelocityField.width / windowWidth
    vec4 grav = texture ( uVelocityField, velPos ) * uVelocityMultiplier;

    vel.x += grav.x;
    vel.y += grav.y;
    vel.z = 1.0 / 60.0;

    pos.xyz += vel.xyz;
    
    if ( pos.z >= pos.w )
    {
    	pos = texture ( uOriginalPositions, UV );
    	pos.z = 0;
    	vel = texture ( uOriginalVelocities, UV );
    }

    NewPosition = pos + vel;
    NewPosition.w = pos.w;
    NewVelocity = vel * uVelocityDamping;
}