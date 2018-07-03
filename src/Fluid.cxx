//
//  Fluid.cxx
//  Fluid
//
//  Created by Andrew Wright on 29/6/17.
//
//

#include "Fluid.h"
#include "CinderImGui.h"

using namespace ci;
static float kFluidScale = 0.5f; // @TODO(andrew): HACK HACK HACK

namespace Fluid
{
    //
    // Common Utils
    //
    
    static const float kReferenceScale = 0.5f; // @TODO(andrew): HACK HACK HACK. Write this into the file somewhere.
    
    template <typename T>
    T ScaleDownToReference ( const T& value )
    {
        float adjustmentScale = kFluidScale / kReferenceScale;
        return value * adjustmentScale;
    }
    
    template <typename T>
    T ScaleUpToReference ( const T& value )
    {
        float adjustmentScale = kFluidScale / kReferenceScale;
        return value / adjustmentScale;
    }
    
    static int kFrame = 0;
    
    Force::Force ( const JsonTree& tree, const vec2& size )
    {
        Position.x = tree["Position.x"].getValue<float>() * size.x;
        Position.y = tree["Position.y"].getValue<float>() * size.y;
        
        Velocity.x = tree["Velocity.x"].getValue<float>();
        Velocity.y = tree["Velocity.y"].getValue<float>();
        
        Color.r = tree["Color.r"].getValue<float>();
        Color.g = tree["Color.g"].getValue<float>();
        Color.b = tree["Color.b"].getValue<float>();
        
        Radius = tree["Radius"].getValue<float>();
        Temperature = tree["Temperature"].getValue<float>();
        Density = tree["Density"].getValue<float>();
        
        Radius = ScaleDownToReference ( Radius );
    }
    
    JsonTree Force::ToJson ( const ci::vec2& size ) const
    {
        JsonTree tree;
        
        JsonTree position = JsonTree::makeObject( "Position" );
        position.pushBack( JsonTree ( "x", Position.x / size.x ) );
        position.pushBack( JsonTree ( "y", Position.y / size.y ) );
        
        JsonTree velocity = JsonTree::makeObject( "Velocity" );
        velocity.pushBack( JsonTree ( "x", Velocity.x ) );
        velocity.pushBack( JsonTree ( "y", Velocity.y ) );
        
        JsonTree color = JsonTree::makeObject( "Color" );
        color.pushBack( JsonTree ( "r", Color.r ) );
        color.pushBack( JsonTree ( "g", Color.g ) );
        color.pushBack( JsonTree ( "b", Color.b ) );
        
        tree.pushBack( position );
        tree.pushBack( velocity );
        tree.pushBack( color );
        tree.pushBack( JsonTree ( "Radius", ScaleUpToReference ( Radius ) ) );
        tree.pushBack( JsonTree ( "Temperature", Temperature ) );
        tree.pushBack( JsonTree ( "Density", Density ) );
        
        return tree;
    }
    
    ScopedFboDraw::ScopedFboDraw ( const ci::gl::FboRef& buffer )
    : _buffer ( buffer )
    {
        gl::pushMatrices();
        gl::pushViewport( ivec2 ( 0 ), _buffer->getSize() );
        gl::setMatricesWindow ( buffer->getSize() );
        gl::context()->pushFramebuffer( buffer );
    }
    
    ScopedFboDraw::ScopedFboDraw( const PingPongBuffer& buffer )
    : _buffer( buffer.DestinationBuffer() )
    {
        gl::pushMatrices();
        gl::pushViewport( ivec2 ( 0 ), _buffer->getSize() );
        gl::setMatricesWindow ( buffer.DestinationBuffer()->getSize() );
        gl::context()->pushFramebuffer( buffer.DestinationBuffer() );
    }
    
    ScopedFboDraw::~ScopedFboDraw ( )
    {
        gl::popMatrices();
        gl::popViewport();
        gl::context()->popFramebuffer();
    }
    
    SimRef Sim::Create ( int width, int height, float scale )
    {
        return SimRef ( new Sim ( width, height, scale ) );
    }
    
    Sim::Sim ( int width, int height, float scale )
    : _sequencer ( Time::Sequencer::Default() )
    {
        _presentShader = gl::GlslProg::create ( app::loadAsset ( "Shaders/Rendering/MatCap.vs.glsl" ), app::loadAsset ( "Shaders/Rendering/MatCap.fs.glsl" ) );
        _presentShader->uniform ( "uDensity", 0 );
        _presentShader->uniform ( "uVelocity", 1 );
        _presentShader->uniform ( "uMatCap", 2 );
        
        _matCapTexture = gl::Texture::create ( loadImage ( app::loadAsset( "mirrored.jpg" ) ) );
        _matCapTexture->setWrap ( GL_MIRRORED_REPEAT, GL_MIRRORED_REPEAT );
        
        _cellSize = 1.25f;
        _gradientScale = 1.00f / _cellSize;
        _numJacobiIterations = 40;
        _timeStep = 0.125f;
        
        DensityDissipation = 0.990f;
        VelocityDissipation = 0.994f;
        TemperatureDissipation = 0.99f;
        PressureDissipation = 0.9f;
        
        SmokeBuoyancy = 1.0f;
        SmokeWeight = 0.05f;
        AmbientTemperature = 0.0f;
        Gravity = vec2 ( 0, -0.98 );
        
        _size.x = width;
        _size.y = height;
        _scale = scale;
        
        kFluidScale = _scale; // @UGH
        
        _gridWidth  = width * scale;
        _gridHeight = height * scale;
        
        LoadShaders();
        
        auto tFmtBase = gl::Texture::Format().target( GL_TEXTURE_RECTANGLE ).wrap( GL_CLAMP_TO_BORDER ).minFilter ( GL_LINEAR ).magFilter ( GL_LINEAR );
        
        auto densityFmt = tFmtBase;
        densityFmt.internalFormat( GL_RGBA32F );
        
        auto velocityFmt = tFmtBase;
        velocityFmt.internalFormat( GL_RGB32F );
        
        auto temperatureFmt = tFmtBase;
        temperatureFmt.internalFormat( GL_R32F );
        
        auto pressureFmt = temperatureFmt;
        
        auto obstacleFmt = tFmtBase;
        obstacleFmt.internalFormat( GL_RGB32F );
        
        auto divergenceFmt = tFmtBase;
        divergenceFmt.internalFormat( GL_R32F );
        
        auto colorAddFmt = densityFmt;
        auto velocityAddFmt = tFmtBase;
        velocityAddFmt.internalFormat( GL_RGB32F );
        
        int samples = 0;
        
        _densityBuffer = PingPongBuffer::Create ( _gridWidth, _gridHeight, gl::Fbo::Format().colorTexture( densityFmt ).disableDepth().samples(samples) );
        _velocityBuffer = PingPongBuffer::Create ( _gridWidth, _gridHeight, gl::Fbo::Format().colorTexture( velocityFmt ).disableDepth().samples(samples) );
        _temperatureBuffer = PingPongBuffer::Create ( _gridWidth, _gridHeight, gl::Fbo::Format().colorTexture( temperatureFmt ).disableDepth().samples(samples) );
        _pressureBuffer = PingPongBuffer::Create ( _gridWidth, _gridHeight, gl::Fbo::Format().colorTexture( pressureFmt ).disableDepth().samples(samples) );
        _obstacleBuffer = gl::Fbo::create( _gridWidth, _gridHeight, gl::Fbo::Format().colorTexture( obstacleFmt ).disableDepth().samples(samples) );
        _divergenceBuffer = gl::Fbo::create( _gridWidth, _gridHeight, gl::Fbo::Format().colorTexture( divergenceFmt ).disableDepth().samples(samples) );
        _colorAddBuffer = gl::Fbo::create ( _gridWidth, _gridHeight, gl::Fbo::Format().colorTexture( colorAddFmt ).disableDepth().samples(samples) );
        _velocityAddBuffer = gl::Fbo::create ( _gridWidth, _gridHeight, gl::Fbo::Format().colorTexture( velocityAddFmt ).disableDepth().samples(samples) );
        
        ClearBuffer( _obstacleBuffer );
        ClearBuffer( _divergenceBuffer );
        ClearBuffer( _colorAddBuffer );
        ClearBuffer( _velocityAddBuffer );

        _temperatureBuffer->Clear ( ColorAf::gray( AmbientTemperature.ValueAtFrame(0) ) );
        
        Clear();
    }
    
    void Sim::LoadShaders ( )
    {
        std::cout << "Loading Shaders\n";
        {
            _advectShader = gl::GlslProg::create ( app::loadAsset ( "Shaders/Fluid/Passthrough.vs.glsl" ), app::loadAsset ( "Shaders/Fluid/Advect.fs.glsl") );
            _advectShader->uniform ( "uVelocityBuffer", 0 );
            _advectShader->uniform ( "uSourceBuffer", 1 );
            _advectShader->uniform ( "uObstacleBuffer", 2 );
        }
        
        {
            _jacobiShader = gl::GlslProg::create ( app::loadAsset ( "Shaders/Fluid/Passthrough.vs.glsl" ), app::loadAsset ( "Shaders/Fluid/Jacobi.fs.glsl") );
            _jacobiShader->uniform ( "uPressureBuffer", 0 );
            _jacobiShader->uniform ( "uDivergenceBuffer", 1 );
            _jacobiShader->uniform ( "uObstacleBuffer", 2 );
            _jacobiShader->uniform ( "uAlpha", -_cellSize * _cellSize );
            _jacobiShader->uniform ( "uInverseBeta", 0.25f );
        }
        
        {
            _subtractGradientShader = gl::GlslProg::create ( app::loadAsset ( "Shaders/Fluid/Passthrough.vs.glsl" ), app::loadAsset ( "Shaders/Fluid/SubtractGradient.fs.glsl") );
            _subtractGradientShader->uniform ( "uVelocityBuffer", 0 );
            _subtractGradientShader->uniform ( "uPressureBuffer", 1 );
            _subtractGradientShader->uniform ( "uObstacleBuffer", 2 );
            _subtractGradientShader->uniform ( "uGradientScale", _gradientScale );
        }
        
        {
            _computeDivergenceShader = gl::GlslProg::create ( app::loadAsset ( "Shaders/Fluid/Passthrough.vs.glsl" ), app::loadAsset ( "Shaders/Fluid/ComputeDivergence.fs.glsl") );
            _computeDivergenceShader->uniform ( "uVelocityBuffer", 0 );
            _computeDivergenceShader->uniform ( "uObstacleBuffer", 1 );
            _computeDivergenceShader->uniform ( "uHalfInverseCellSize", 0.5f / _cellSize );
        }
        
        {
            _applyImpulseShader = gl::GlslProg::create ( app::loadAsset ( "Shaders/Fluid/Passthrough.vs.glsl" ), app::loadAsset ( "Shaders/Fluid/ApplyImpulse.fs.glsl") );
        }
        
        {
            _applyTextureShader = gl::GlslProg::create ( app::loadAsset ( "Shaders/Fluid/Passthrough.vs.glsl" ), app::loadAsset ( "Shaders/Fluid/ApplyTexture.fs.glsl") );
            _applyTextureShader->uniform( "uSourceBuffer", 0 );
            _applyTextureShader->uniform( "uTexture", 1 );
        }
        
        {
            _applyBuoyancyShader = gl::GlslProg::create ( app::loadAsset ( "Shaders/Fluid/Passthrough.vs.glsl" ), app::loadAsset ( "Shaders/Fluid/ApplyBuoyancy.fs.glsl") );
            _applyBuoyancyShader->uniform( "uVelocityBuffer", 0 );
            _applyBuoyancyShader->uniform( "uTemperatureBuffer", 1 );
            _applyBuoyancyShader->uniform( "uDensityBuffer", 2 );
        }
    }
    
    void Sim::ClearBuffer ( const gl::FboRef& buffer, const ColorAf& clearColor )
    {
        ScopedFboDraw draw { buffer };
        gl::clear ( clearColor );
    }
    
    Surface8u Sim::GetDensityEdge ( float y0, float y1 ) const
    {
        auto b = _densityBuffer->DestinationBuffer()->getBounds();
        float h = b.getHeight();
        
        b.y2 = ( 1.0 - y0 ) * h;
        b.x1 = b.x2 - 2;
        b.y1 = ( 1.0 - y1 ) * h;
        
        return _densityBuffer->DestinationBuffer()->readPixels8u( b );
    }
    
    void Sim::EnableObstacles ( bool enabled )
    {
        _obstaclesEnabled = enabled;
        if ( !enabled )
        {
            ClearBuffer ( _obstacleBuffer );
        }
    }
    
    void Sim::AddConstantForce ( const Force& force )
    {
        _constantForces.push_back( force );
        _constantForces.back().Position *= _scale;
    }
    
    void Sim::AddTemporalForce ( const Force& force )
    {
        _temporalForces.push_back( force );
        _temporalForces.back().Position *= _scale;
        _temporalForces.back().Radius   *= _scale;
    }
    
    void Sim::Clear ( float clearAlpha )
    {
        _densityBuffer->Clear();
        _velocityBuffer->Clear();
        _temperatureBuffer->Clear( ColorAf::gray( AmbientTemperature.ValueAtFrame(0) ) );
        _pressureBuffer->Clear();
        
        ClearBuffer( _obstacleBuffer );
        ClearBuffer( _divergenceBuffer );
        ClearBuffer( _colorAddBuffer, ColorAf ( 0, 0, 0, clearAlpha ) );
        ClearBuffer( _velocityAddBuffer );
    }
    
    void Sim::Inspect ( )
    {
        if ( ui::CollapsingHeader( "Rendering Params" ) )
        {
            ui::ScopedId id { "FluidRenderParams" };
            Time::Inspect ( Alpha, "Alpha", Time::BetweenZeroAndOne );
            Time::Inspect ( Metalness, "Metalness", Time::BetweenZeroAndOne );
            Time::Inspect ( _matCapPerturbation, "Perturbation" );
            Time::Inspect ( _matCapExponent, "Exponent" );
        }
        
        if ( ui::CollapsingHeader( "Simulation Params" ) )
        {
            ui::ScopedId id { "FluidSimParams" };
            if ( ui::DragFloat ( "Time Step", &_timeStep, 0.001f, 0.0f, 2.0f ) ) { }
            if ( ui::DragInt ( "Jacobi Iterations", &_numJacobiIterations, 0, 1, 100 ) ) { }
            
            Time::Inspect ( Gravity, "Gravity" );
            Time::Inspect ( SmokeBuoyancy, "Smoke Bouyancy" );
            Time::Inspect ( SmokeWeight, "Smoke Weight" );
        }
        
        if ( ui::CollapsingHeader( "Dissipation" ) )
        {
            ui::ScopedId id { "FluidDissipation" };
            if ( ui::DragFloat ( "Velocity", &VelocityDissipation, 0.001f, 0.0f, 0.9999f ) ) { }
            if ( ui::DragFloat ( "Density", &DensityDissipation, 0.001f, 0.0f, 0.9999f ) ) { }
            if ( ui::DragFloat ( "Temperature", &TemperatureDissipation, 0.001f, 0.0f, 0.9999f ) ) { }
            if ( ui::DragFloat ( "Pressure", &PressureDissipation, 0.001f, 0.0f, 0.9999f ) ) { }
        }
    }
    
    void Sim::UpdateAttractors ( )
    {
        auto attrs = _sequencer.GetAttractors();
        float t = _sequencer.Time();
        
        for ( int i = 0; i < 4; i++ )
        {
            bool enabled = i < attrs.size();
            
            std::string prefix = "uAttractors[" + std::to_string(i) + "].";
            _applyBuoyancyShader->uniform ( prefix + "Enabled", enabled );
            
            if ( enabled )
            {
                auto& a = attrs[i];
                _applyBuoyancyShader->uniform ( prefix + "Radius", a->RadiusAt(t) * Scale() );
                _applyBuoyancyShader->uniform ( prefix + "Force", a->ForceAt(t) ); // * ( a.Attract ? 1.0f : -1.0f ) );
                _applyBuoyancyShader->uniform ( prefix + "Position", a->PositionAt(t) * Scale() );
            }
        }
    }
    
    void Sim::DrawBuffers ( )
    {
        gl::ScopedState blend { GL_BLEND, false };
        
        std::vector<std::pair<std::string, gl::TextureRef>> buffers =
        {
            { "Velocity", _velocityBuffer->SourceTexture() },
            { "Temperature", _temperatureBuffer->SourceTexture() },
            { "Pressure", _pressureBuffer->SourceTexture() },
            { "Density",  _densityBuffer->SourceTexture() },
            { "Divergence", _divergenceBuffer->getColorTexture() },
            { "Obstacles", _obstacleBuffer->getColorTexture() }
        };
        
        static gl::TextureFontRef kFont = gl::TextureFont::create( Font ( app::loadAsset( "04b11.ttf" ), 8 ) );
        
        Rectf r { vec2(0), vec2(app::getWindowSize()) / (float)buffers.size() };
        
        for ( auto& t : buffers )
        {
            gl::draw ( t.second, r );
            gl::drawStrokedRect( r );
            
            gl::ScopedBlendAlpha blend;
            kFont->drawString( t.first, r.getLowerLeft() + vec2 ( 8 ) );
            
            r += vec2 ( r.getWidth(), 0 );
		}
    }
    
    void Sim::ApplyForces ( )
    {
        for ( auto& force : _temporalForces )
        {
            gl::enableAdditiveBlending();
            gl::disable ( GL_BLEND );
            ApplyImpulse( *_temperatureBuffer.get(), vec3(force.Position, 0), vec3(force.Temperature), force.Radius );
            if ( force.Color != Colorf::black() )
            {
                vec3 color { force.Color.r, force.Color.g, force.Color.b };
                ApplyImpulse( *_densityBuffer.get(), vec3(force.Position, 0), color * force.Density, force.Radius );
            }
            
            if ( glm::length( force.Velocity ) != 0 )
            {
                ApplyImpulse( *_velocityBuffer.get(), vec3(force.Position, 0), vec3(force.Velocity, 0), force.Radius );
            }
        }
        
        _temporalForces.clear();
        
        for ( auto& force : _constantForces )
        {
            gl::enableAdditiveBlending();
            gl::disable ( GL_BLEND );
            ApplyImpulse( *_temperatureBuffer.get(), vec3(force.Position, 0), vec3(force.Temperature), force.Radius );
            if ( force.Color != Colorf::black() )
            {
                vec3 color { force.Color.r, force.Color.g, force.Color.b };
                ApplyImpulse( *_densityBuffer.get(), vec3(force.Position, 0), color * force.Density, force.Radius );
            }
            
            if ( glm::length( force.Velocity ) != 0 )
            {
                ApplyImpulse( *_velocityBuffer.get(), vec3(force.Position, 0), vec3(force.Velocity, 0), force.Radius );
            }
        }
    }
    
    void Sim::Update ( double dt )
    {
        kFrame++;
        
        if ( _obstaclesEnabled && ObstaclesDirty && ObstacleRenderHandler )
        {
            ScopedFboDraw draw { _obstacleBuffer };
            gl::ScopedBlendAlpha blend;
            gl::clear();
            
            ObstacleRenderHandler ( _obstacleBuffer->getBounds(), false );
            ObstaclesDirty = false;
        }
        
        ApplyForces();

        gl::disableAlphaBlending();
        gl::disable ( GL_BLEND );
        
        Advect ( *_velocityBuffer.get(), VelocityDissipation );
        _velocityBuffer->Swap();
        
        Advect ( *_temperatureBuffer.get(), TemperatureDissipation );
        _temperatureBuffer->Swap();
       
        Advect ( *_densityBuffer.get(), DensityDissipation );
        _densityBuffer->Swap();
        
        ApplyBuoyancy();
        _velocityBuffer->Swap();
        
        gl::enableAlphaBlending();
        gl::disable( GL_BLEND );
        
        ComputeDivergence ( );
        ClearBuffer( _pressureBuffer->SourceBuffer() );
        
        for ( int i = 0; i < _numJacobiIterations; i++ )
        {
            Jacobi (  ) ;
            _pressureBuffer->Swap();
        }
        
        SubtractGradient ( );
        _velocityBuffer->Swap();
        
        UpdateAttractors ( );
    }
    
    void Sim::Draw ( const Rectf& bounds )
    {
        float t = _sequencer.Time();
        float a = Alpha.ValueAtTime( t );
        if ( a > 0.0f )
        {
            gl::ScopedGlslProg shader { _presentShader };
            gl::ScopedTextureBind tex0 ( _densityBuffer->SourceTexture(), 0 );
            gl::ScopedTextureBind tex1 ( _velocityBuffer->SourceTexture(), 1 );
            gl::ScopedTextureBind tex2 ( _matCapTexture, 2 );
            
            _presentShader->uniform( "uSceneScale", vec2 ( GetDensity()->getSize() ) );
            _presentShader->uniform( "uMetalness", Metalness.ValueAtTime ( t ) );
            _presentShader->uniform( "uDensityColor", ColorAf ( a, a, a, a ) );
            _presentShader->uniform( "uPerturbation", _matCapPerturbation.ValueAtTime(t) );
            _presentShader->uniform( "uPerturbationExponent", _matCapExponent.ValueAtTime(t) );
            
            Rectf r = bounds;
            std::swap(r.y1, r.y2);
            
            gl::drawSolidRect( r );
        }
    }
    
    void Sim::DrawVelocity ( const Rectf& bounds )
    {
        _velocityBuffer->Draw ( bounds );
    }
    
    void Sim::Advect ( PingPongBuffer& buffer, float dissipation ) const
    {
        auto& prog = _advectShader;
        
        ScopedFboDraw ping { buffer };
        gl::ScopedGlslProg shader { prog };
        gl::ScopedTextureBind tex0 { _velocityBuffer->SourceTexture(), 0 };
        gl::ScopedTextureBind tex1 { buffer.SourceTexture(), 1 };
        gl::ScopedTextureBind tex2 { _obstacleBuffer->getColorTexture(), 2 };
        
        prog->uniform ( "uTimeStep", _timeStep );
        prog->uniform ( "uDissipation", dissipation );
        
        RenderQuad ( _gridWidth, _gridHeight );
        ResetGLState ( );
    }
    
    void Sim::Jacobi ( ) const
    {
        auto& prog = _jacobiShader;
        
        ScopedFboDraw ping { *_pressureBuffer.get() };
        gl::ScopedGlslProg shader { prog };
        gl::ScopedTextureBind tex0 { _pressureBuffer->SourceTexture(), 0 };
        gl::ScopedTextureBind tex1 { _divergenceBuffer->getColorTexture(), 1 };
        gl::ScopedTextureBind tex2 { _obstacleBuffer->getColorTexture(), 2 };
        
        RenderQuad ( _gridWidth, _gridHeight );
        ResetGLState ( );
    }
   
    void Sim::SubtractGradient ( ) const
    {
        auto& prog = _subtractGradientShader;
        
        ScopedFboDraw ping { *_velocityBuffer.get() };
        gl::ScopedGlslProg shader { prog };
        gl::ScopedTextureBind tex0 { _velocityBuffer->SourceTexture(), 0 };
        gl::ScopedTextureBind tex1 { _pressureBuffer->SourceTexture(), 1 };
        gl::ScopedTextureBind tex2 { _obstacleBuffer->getColorTexture(), 2 };
        
        RenderQuad ( _gridWidth, _gridHeight );
        ResetGLState ( );
    }
    
    void Sim::ComputeDivergence ( ) const
    {
        auto& prog = _computeDivergenceShader;
        
        ScopedFboDraw buffer { _divergenceBuffer };
        gl::ScopedGlslProg shader { prog };
        gl::ScopedTextureBind tex0 { _velocityBuffer->SourceTexture(), 0 };
        gl::ScopedTextureBind tex1 { _obstacleBuffer->getColorTexture(), 1 };
        
        RenderQuad ( _gridWidth, _gridHeight );
        ResetGLState ( );
    }
    
    void Sim::ApplyImpulse ( PingPongBuffer& buffer, const gl::TextureRef& texture, float weight, bool isVelocity )
    {
        {
            auto& prog = _applyTextureShader;
            
            ScopedFboDraw ping { buffer.DestinationBuffer() };
            gl::ScopedGlslProg shader { prog };
            gl::ScopedTextureBind tex0 ( buffer.SourceTexture(), 0 );
            gl::ScopedTextureBind tex1 ( texture, 1 );
            
            prog->uniform ( "uWeight", weight );
            prog->uniform ( "uIsVelocity", isVelocity ? 1 : 0 );
            
            gl::ScopedState blend { GL_BLEND, GL_TRUE };
            RenderQuad( _gridWidth, _gridHeight );
        }
        
        buffer.Swap();
    }
    
    void Sim::ApplyImpulse ( PingPongBuffer& buffer, const vec3& force, const vec3& value, float radius )
    {
        {
            auto& prog = _applyImpulseShader;
            
            ScopedFboDraw ping { buffer.SourceBuffer() };
            gl::ScopedGlslProg shader { prog };
            
            prog->uniform ( "uPoint", vec2 ( force ) );
            prog->uniform ( "uRadius", radius );
            prog->uniform ( "uValue", value );
            
            gl::ScopedState blend { GL_BLEND, GL_TRUE };
            RenderQuad( _gridWidth, _gridHeight );
        }
    }
    
    void Sim::ApplyBuoyancy ( )
    {
        auto& prog = _applyBuoyancyShader;
        
        ScopedFboDraw ping { _velocityBuffer->DestinationBuffer() };
        gl::ScopedGlslProg shader { prog };
        gl::ScopedTextureBind tex0 { _velocityBuffer->SourceTexture(), 0 };
        gl::ScopedTextureBind tex1 { _temperatureBuffer->SourceTexture(), 1 };
        gl::ScopedTextureBind tex2 { _densityBuffer->SourceTexture(), 2 };
        
        float t = _sequencer.Time();
        
        prog->uniform ( "uAmbientTemperature", AmbientTemperature.ValueAtTime(t) );
        prog->uniform ( "uTimeStep", _timeStep );
        prog->uniform ( "uSigma", SmokeBuoyancy.ValueAtTime(t) );
        prog->uniform ( "uKappa", SmokeWeight.ValueAtTime(t) );
        prog->uniform ( "uGravity", Gravity.ValueAtTime(t) );
        
        RenderQuad ( _gridWidth, _gridHeight );
        ResetGLState();
    }
    
    void Sim::RenderQuad ( int width, int height ) const
    {
        gl::drawSolidRect( Rectf ( 0, 0, width, height ), vec2 ( 0, height ), vec2 ( width, 0 ) );
    }
    
    void Sim::ResetGLState ( ) const
    {
        gl::disable ( GL_BLEND );
    }
}
