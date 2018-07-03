//
//  ParticleSystem.h
//  BlackHole
//
//  Created by Andrew Wright on 27/7/17.
//
//

#ifndef BlackHole_ParticleSystem_h
#define BlackHole_ParticleSystem_h

#include "cinder/Cinder.h"
#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/Json.h"

#include <Time/Property.h>

using ParticleSystemRef = std::unique_ptr<class ParticleSystem>;
class ParticleSystem
{
public:
    
    ParticleSystem      ( );
    
    void                Init    ( int resPowerOf2 );
    void                Update  ( float dt, const ci::gl::Texture2dRef& velocityField );
    void                Inspect ( );
    void                Draw    ( const ci::gl::Texture2dRef& densityField );
    
    void                Load    ( const ci::JsonTree& tree );
    void                Save    ( ci::JsonTree& tree );
    
    float               Scale{0.5f};

	float				DensityAlphaMultiplier{1.0f};

    Time::FloatProperty Alpha{1.0f};
    Time::FloatProperty AlphaMin{0.0f};
    Time::FloatProperty AlphaMultiplier{0.8f};
    Time::FloatProperty MaxParticleSize{16.0f};
    Time::FloatProperty VelocityDamping{0.96f};
    Time::FloatProperty VelocityMultiplier{0.004f};
    
protected:
    
    int                 _read{0};
    int                 _write{1};
    
    ci::gl::TextureRef  _positions[2];
    ci::gl::TextureRef  _velocities[2];
    ci::gl::TextureRef  _originalPositions;
    ci::gl::TextureRef  _originalVelocities;
    ci::gl::TextureRef  _particle;
    
    ci::gl::FboRef      _buffers[2];
    
    ci::gl::GlslProgRef _simShader;
    ci::gl::GlslProgRef _renderShader;
    
    ci::gl::VboMeshRef  _pointMesh;
    
    int                 _res{8};
    float               _alphaMin{0.0f};
    float               _alphaMultiplier{0.8f};
    float               _maxParticleSize{16.0f};
    float               _velocityDamping{0.96f};
    float               _velocityMultiplier{0.004f};
};

#endif /* BlackHole_ParticleSystem_h */
