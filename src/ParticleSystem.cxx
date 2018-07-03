//
//  ParticleSystem.cxx
//  BlackHole
//
//  Created by Andrew Wright on 27/7/17.
//
//

#include "ParticleSystem.h"
#include "cinder/Rand.h"
#include "CinderImGui.h"
#include <Time/Sequencer.h>

using namespace ci;

ParticleSystem::ParticleSystem (  )
{
    {
        auto fmt = gl::GlslProg::Format().vertex( app::loadAsset( "Shaders/Particles/ParticleSim.vs.glsl" ) ).fragment ( app::loadAsset( "Shaders/Particles/ParticleSim.fs.glsl" ) );
        fmt.fragDataLocation( 0, "NewPosition" );
        fmt.fragDataLocation( 1, "NewVelocity" );
        
        _simShader = gl::GlslProg::create( fmt );
        _simShader->uniform ( "uPositionBuffer", 0 );
        _simShader->uniform ( "uVelocityBuffer", 1 );
        _simShader->uniform ( "uOriginalPositions", 2 );
        _simShader->uniform ( "uOriginalVelocities", 3 );
        _simShader->uniform ( "uVelocityField", 4 );
    }
    
    {
        auto fmt = gl::GlslProg::Format().vertex( app::loadAsset( "Shaders/Particles/ParticleRender.vs.glsl" ) ).fragment ( app::loadAsset( "Shaders/Particles/ParticleRender.fs.glsl" ) );
        
        _renderShader = gl::GlslProg::create( fmt );
        _renderShader->uniform ( "uPositionBuffer", 0 );
        _renderShader->uniform ( "uVelocityBuffer", 1 );
        _renderShader->uniform ( "uParticle", 2 );
        _renderShader->uniform ( "uDensity", 3 );
        _renderShader->uniform ( "uGlobalAlpha", Alpha.ValueAtFrame(0) );
    }
    
    CI_CHECK_GL();
}

void ParticleSystem::Init ( int resPowerOf2 )
{
    int res = std::pow ( 2, resPowerOf2 );
    
    Surface32f positions { res, res, true };
    Surface32f velocities { res, res, true };
    std::vector<vec2> uvs;
    
    float u = 0.0f, v = 0.0f;
    float us = 1.0 / (float)res, vs = 1.0 / (float)res;
    
    for ( int i = 0; i < res; i++ )
    {
        v = 0.0f;
        for ( int j = 0; j < res; j++ )
        {
            vec2 pos { randFloat(), randFloat() };
            pos *= vec2 ( app::getWindowSize() );
            
            vec2 vel { randFloat(-1, 1), randFloat(-1, 1) };
            vel *= 0.0f;
            
            positions.setPixel( ivec2 ( i, j ), ColorAf ( pos.x, pos.y, 0, 1.0 + randFloat(8.0) ) );
            velocities.setPixel( ivec2 ( i, j ), ColorAf ( vel.x, vel.y, 0, 1 ) );
            
            uvs.push_back( vec2 ( u, v ) );
            v += vs;
        }
        u += us;
    }
    
    _particle = gl::Texture::create( loadImage( app::loadAsset( "Particle.png" ) ), gl::Texture::Format().minFilter(GL_LINEAR).magFilter(GL_LINEAR) );
    
    _pointMesh = gl::VboMesh::create( static_cast<int>(uvs.size()), GL_POINTS, { gl::VboMesh::Layout().attrib( geom::TEX_COORD_0, 2 ) } );
    _pointMesh->bufferAttrib( geom::TEX_COORD_0, uvs );
    
    auto tFmt = gl::Texture::Format().internalFormat(GL_RGBA32F).minFilter(GL_NEAREST).magFilter(GL_NEAREST).dataType( GL_FLOAT );
    
    _originalPositions = gl::Texture::create( positions, tFmt );
    _originalVelocities = gl::Texture::create( velocities, tFmt );

    _positions[0] = gl::Texture::create( positions, tFmt );
    _positions[1] = gl::Texture::create( res, res, tFmt );
    
    _velocities[0] = gl::Texture::create( velocities, tFmt );
    _velocities[1] = gl::Texture::create( res, res, tFmt );
    
    auto fbo0 = gl::Fbo::Format().attachment ( GL_COLOR_ATTACHMENT0, _positions[0] )
                                 .attachment ( GL_COLOR_ATTACHMENT1, _velocities[0] )
                                 .disableColor().disableDepth().samples(0);
    
    auto fbo1 = gl::Fbo::Format().attachment ( GL_COLOR_ATTACHMENT0, _positions[1] )
                                 .attachment ( GL_COLOR_ATTACHMENT1, _velocities[1] )
                                 .disableColor().disableDepth().samples(0);
    
    _buffers[0] = gl::Fbo::create( res, res, fbo0 );
    _buffers[1] = gl::Fbo::create( res, res, fbo1 );
    _res = resPowerOf2;
    
    CI_CHECK_GL();
}

void ParticleSystem::Load ( const JsonTree& tree )
{
    if ( tree.hasChild ( "Particles" ) )
    {
        _res = tree["Particles.ResPowOf2"].getValue<int>();
        _velocityDamping = tree["Particles.VelocityDamping"].getValue<float>();
        _velocityMultiplier = tree["Particles.VelocityMultiplier"].getValue<float>();
        _maxParticleSize = tree["Particles.MaxParticleSize"].getValue<float>();
        
        Init ( _res );
    }
}

void ParticleSystem::Save ( JsonTree& tree )
{
    JsonTree node = JsonTree::makeObject ( "Particles" );
    node.pushBack( JsonTree ( "ResPowOf2", _res ) );
    node.pushBack( JsonTree ( "VelocityDamping", _velocityDamping ) );
    node.pushBack( JsonTree ( "VelocityMultiplier", _velocityMultiplier ) );
    node.pushBack( JsonTree ( "MaxParticleSize", _maxParticleSize ) );
    tree.pushBack( node );
}

void ParticleSystem::Update ( float dt, const ci::gl::Texture2dRef& velocityField )
{
    auto t = Time::Sequencer::Default().Time();
    float alpha = Alpha.ValueAtTime(t);
    float size = MaxParticleSize.ValueAtTime(t);
    
    _renderShader->uniform ( "uGlobalAlpha", Alpha.ValueAtTime(t));
	_renderShader->uniform ( "uDensityContribution", DensityAlphaMultiplier );
    
    if ( alpha > 0.0f && size > 0.0f )
    {
        gl::ScopedFramebuffer buffer { _buffers[_write] };
        gl::ScopedGlslProg shader { _simShader };
        
        gl::enable ( GL_VERTEX_PROGRAM_POINT_SIZE );
        gl::ScopedDepth depth { false };
        gl::ScopedMatrices m;
        gl::disableAlphaBlending();
        gl::clear();
       
        gl::setMatricesWindow ( _buffers[_write]->getSize() );
        gl::ScopedViewport vp { ivec2(0), _buffers[_write]->getSize() };
        
        gl::ScopedTextureBind tex0 ( _positions[_read], 0 );
        gl::ScopedTextureBind tex1 ( _velocities[_read], 1 );
        gl::ScopedTextureBind tex2 ( _originalPositions, 2 );
        gl::ScopedTextureBind tex3 ( _originalVelocities, 3 );
        gl::ScopedTextureBind tex4 ( velocityField, 4 );
        
        gl::drawSolidRect( _buffers[_write]->getBounds() );
        
        std::swap ( _read, _write );
    }
}

void ParticleSystem::Inspect ( )
{
    if ( ui::CollapsingHeader( "Particle System" ) )
    {
        ui::ScopedId id { "ParticleSystem" };
        int p = std::pow(2, _res);
        ui::Text( "%d Particles", p * p);
        if ( ui::DragInt( "Particle Count", &_res, 0, 1, 10 ) )
        {
            Init ( _res );
        }
        
        Time::Inspect ( Alpha, "Alpha", Time::BetweenZeroAndOne );
        
        ui::Indent();
        if ( ui::CollapsingHeader ( "Simulation" ) )
        {
            Time::Inspect ( VelocityDamping, "Velocity Damping", Time::BetweenZeroAndOne );
            Time::Inspect ( VelocityMultiplier, "Velocity Multiplier", Time::BetweenZeroAndOne );
        }
        
        if ( ui::CollapsingHeader ( "Rendering" ) )
        {
            Time::Inspect ( AlphaMin, "Alpha Minimum", Time::BetweenZeroAndOne );
            Time::Inspect ( AlphaMultiplier, "Alpha Multiplier", Time::BetweenZeroAndOne );
            Time::Inspect ( MaxParticleSize, "Max Particle Size" );
        }
        
        ui::Unindent();
    }
}

void ParticleSystem::Draw ( const ci::gl::Texture2dRef& densityField )
{
    auto t = Time::Sequencer::Default().Time();
    float alpha = Alpha.ValueAtTime(t);
    float size = MaxParticleSize.ValueAtTime(t);
   
    if ( alpha > 0.0f && size > 0.0f )
    {
        _simShader->uniform( "uVelocityDamping", VelocityDamping.ValueAtTime(t) );
        _simShader->uniform( "uVelocityMultiplier", VelocityMultiplier.ValueAtTime(t) );
        
        _renderShader->uniform ( "uAlphaMin", AlphaMin.ValueAtTime(t) );
        _renderShader->uniform ( "uAlphaMultiplier", AlphaMultiplier.ValueAtTime(t) );
        _renderShader->uniform ( "uMaxParticleSize", size );
        
        gl::ScopedGlslProg shader { _renderShader };
        gl::ScopedBlendAdditive blend;
        gl::ScopedTextureBind tex0 ( _positions[_read], 0 );
        gl::ScopedTextureBind tex1 ( _velocities[_read], 1 );
        gl::ScopedTextureBind tex2 ( _particle, 2 );
        gl::ScopedTextureBind tex3 ( densityField, 3 );
        
        gl::ScopedModelMatrix m;
        gl::scale ( vec2 ( 0.5f / Scale ) );
        gl::draw ( _pointMesh );
    }
}
