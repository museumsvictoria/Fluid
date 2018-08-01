//
//  FluidApp.cxx
//  Fluid
//
//  Created by Andrew Wright on 29/6/17.
//
//

// @TODO(andrew): @PERF Do we need 4 channels % 8bpp? RGB16F vs RGBA32F?

#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "CinderImGui.h"
#include "FluidApp.h"
#include "ImageSequence.h"
#include "cinder/Rand.h"

using namespace ci;
using namespace ci::app;

#ifdef CINDER_COCOA
static uint64_t kLastWrite = 0;
#else
static fs::file_time_type kLastWrite;
#endif

#define STANDALONE_DEMO

namespace
{
    static fs::path          kFolderToWatch = getHomeDirectory() / "Dropbox/scienceworks-build/fluiddynamics/FluidScenes/";
    static std::string       kFileToWatch = "FluidDesigner.json";
    
    vec2                     kPrevMouse;
    vec2                     kMouse;
    
    bool                     kDrawBuffers{false};
    bool                     kMousePaint{false};
    bool                     kDrawAttractors{false};
    
    float                    kScale         = 0.25f;
    
    static std::string       kSmokeOSCAddress;
    static std::string       kMetalOSCAddress;
    static std::string       kFlowOSCAddress;
    static std::string       kParticlesOSCAddress;
}

void FluidApp::Init ( app::App::Settings * settings )
{
    #ifdef CINDER_MAC
    
    #else
    settings->setFullScreen();
    #endif

#ifdef STANDALONE_DEMO
    settings->setMultiTouchEnabled(true);
#endif
}

FluidApp::FluidApp ( )
: _sequencer ( Time::Sequencer::Default() )
{
    #ifdef CINDER_MAC
    getWindow()->spanAllDisplays();
    #endif
    OnSetup ( );
    
    getSignalUpdate().connect ( std::bind ( &FluidApp::OnUpdate, this ) );
    getWindow()->getSignalDraw().connect ( std::bind ( &FluidApp::OnDraw, this ) );
    getWindow()->getSignalKeyDown().connect( std::bind ( &FluidApp::HandleKeyDown, this, std::placeholders::_1 ) );
    getSignalCleanup().connect ( std::bind ( &FluidApp::OnCleanup, this ) );
}

void FluidApp::OnSetup ( )
{
    ui::initialize();

    hideCursor();

    #ifdef CINDER_MSW
    // Disable crash dialog
    DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
    SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
    #endif
    
    _particles.Init( 9 );
    _particles.Alpha.ValueAtFrame(0) = 0.0f;
    
    _encoders = std::make_unique<RotaryEncoders>();
    
    InitFluidAtScale ( kScale );
    
    try
    {
        JsonTree config { loadAsset ( "Config.json" ) };
        
        auto host   = config["OSCEndpoint"].getValue();
        auto port   = config["OSCPort"].getValue<int>();
        auto peerIP = config["PeerIP"].getValue();
        
        _oscChannel = std::make_unique<Time::OSCChannel>( host, port );
        _isLeft = config["IsLeft"].getValue<bool>();
        _edgeInset = config["EdgeInset"].getValue<int>();
        _syncFrameInterval = config["SyncFrameInterval"].getValue<int>();
        _encoderMappings.clear();
        
        Time::Element::kIsLeft = _isLeft;
        
        if ( config.hasChild( "SceneFile" ) )
        {
            kFileToWatch = config["SceneFile"].getValue();
        }
        
        for ( auto& e : config["EncoderMappings"] )
        {
            std::vector<std::string> mappings;
            for ( auto& m : e )
            {
                mappings.push_back ( m.getValue() );
            }
            _encoderMappings.push_back( mappings );
        }
        
        if ( _isLeft )
        {
            _syncTransport = std::make_unique<Time::OSCChannel> ( peerIP, 9889, Time::OSCChannel::Mode::Outgoing );
        }else
        {
            _syncTransport = std::make_unique<Time::OSCChannel> ( "", 9889, Time::OSCChannel::Mode::Incoming );
            _syncTransport->Listen ( [&] ( float time )
            {
                float delta = std::abs ( _sequencer.Time() - time );
                if ( delta > 0.5f )
                {
                    _sequencer.StepTo( time );
                }
            });
        }
        
        {
            std::string dir = _isLeft ? "left" : "right";
            kSmokeOSCAddress = "/bp/source_volume/FD_Smoke_" + dir + "_48k";
            kMetalOSCAddress = "/bp/source_volume/FD_Metal_" + dir + "_48k";
            kFlowOSCAddress = "/bp/source_volume/FD_FlowField_" + dir + "_48k";
            kParticlesOSCAddress = "/bp/source_volume/FD_Particles_" + dir + "_48k";
        }
        
        
    }catch ( const std::exception& e )
    {
        _errorList.push_back( "Error loading config JSON: " + std::string ( e.what() ) );
    }
    
    if ( _isLeft )
    {
        _sequencer.OnLoop ( [&] { _syncTransport->SendEvent( "/sync", _sequencer.Time() ); });
    }

    _flowField->Alpha = 0.6f;
    _flowField->ColorWeight = 0.85f;
    _particles.Alpha = 1.0f; 
    _particles.AlphaMin = 1.0f;
    _fluid->Alpha = 0.115f;
    _fluid->Metalness = 0.0f;

#ifdef STANDALONE_DEMO
    
    for ( int i = 0; i < 8; i++ )
    {
        float a = toRadians ( i * (360.0f / 8.0f) );
        vec2 p;
        p.x = std::cos(a) * _fluid->Size().x * 0.4f;
        p.y = std::sin(a) * _fluid->Size().y * 0.4f;
            
        Fluid::Force f;
        f.Position = vec2(_fluid->Size()) * 0.5f + p;
        f.Density = 1.0f;
        f.Radius = 16.0f;
        f.Velocity = -glm::normalize(p);
        f.Color = Color ( { randFloat(0.1, 0.8), randFloat(0.1, 0.8), randFloat(0.1, 0.8) } );

        _fluid->AddConstantForce ( f );
    }

    getWindow()->getSignalTouchesMoved().connect ( [=] ( app::TouchEvent event )
    {
        for ( auto& touch : event.getTouches() )
        {
            vec2 delta = touch.getPrevPos() - touch.getPos();

            float xPos = lmap(touch.getPos().x, 0.0f, (float)getWindowWidth(), 0.1f, 0.8f);
            float yPos = lmap(touch.getPos().y, 0.0f, (float)getWindowHeight(), 0.1f, 0.8f);

            float blue = std::sin(getElapsedSeconds() * 0.2f) * 0.5f + 0.5f;
            blue = lmap(blue, 0.0f, 1.0f, 0.1f, 0.8f);

            float length = glm::length(delta);

            if (length > 2.0f)
            {
                if (length > 10.0f) delta = glm::normalize(delta) * 10.0f;

                Fluid::Force f;
                f.Position = touch.getPos();
                f.Density = 0.8f;
                f.Radius = 32.0f;
                f.Velocity = -delta;
                f.Color = Color(xPos, yPos, blue);

                _fluid->AddTemporalForce(f);
            }
        }
    });

    getWindow()->getSignalMouseDrag().connect([=](app::MouseEvent event)
    {
        static vec2 prev = event.getPos();
        vec2 now = event.getPos();
        vec2 delta = prev - now;
        prev = now;

        float xPos = lmap(now.x, 0.0f, (float)getWindowWidth(), 0.1f, 0.8f);
        float yPos = lmap(now.y, 0.0f, (float)getWindowHeight(), 0.1f, 0.8f);

        float blue = std::sin(getElapsedSeconds() * 0.2f) * 0.5f + 0.5f;
        blue = lmap(blue, 0.0f, 1.0f, 0.1f, 0.8f);

        float length = glm::length(delta);

        if ( length > 2.0f )
        {
            if ( length > 10.0f ) delta = glm::normalize(delta) * 10.0f;

            Fluid::Force f;
            f.Position = now;
            f.Density = 0.8f;
            f.Radius = 32.0f;
            f.Velocity = -delta;
            f.Color = Color( xPos, yPos, blue );
        
            _fluid->AddTemporalForce ( f );
        }
    });

#endif
}

void FluidApp::InitFluidAtScale ( float scale )
{
    _fluid = Fluid::Sim::Create ( app::getWindowWidth(), app::getWindowHeight(), scale );
    _fluid->DensityDissipation = 0.995;
    _flowField = std::make_unique<FlowField>(_fluid.get());
    
    _fluid->Gravity = vec2(0);
    _fluid->EnableObstacles(true);
    _fluid->ObstacleRenderHandler = [=] ( const Rectf& rect, bool topLeft )
    {
        if ( _fluid->AreObstaclesEnabled() )
        {
            gl::ScopedMatrices m;
            gl::setMatricesWindow ( rect.getSize(), topLeft );
            gl::scale ( vec2 ( rect.getWidth() / (float)getWindowWidth() ) );
            
            // if topLeft it's the display render
            float overhang = topLeft ? 1.035f : 1.0f;
            for ( auto& o : _sequencer.GetObstacles() )
            {
                o->Draw( overhang );
            }
        }
        
        if ( !topLeft && _edgeInset > 0 )
        {
            gl::ScopedMatrices m;
            gl::setMatricesWindow ( rect.getSize(), topLeft );
            
            Rectf box { 0.0f, 0.0f, (float)_edgeInset, (float)_fluid->Size().y };
            gl::drawSolidRect ( box );
            
            box += vec2( rect.getWidth() - _edgeInset, 0 );
            gl::drawSolidRect ( box );
        }
    };
    
    _particles.Scale = scale;
}

void FluidApp::OnUpdate ( )
{
#ifndef STANDALONE_DEMO
    if ( app::getElapsedFrames() == 1 || app::getElapsedFrames() % 60 == 0 )
    {
        try
        {
            auto fullFile = kFolderToWatch / kFileToWatch;
            if ( fs::exists ( fullFile ) )
            {
                auto n = fs::last_write_time( fullFile );
                if ( n != kLastWrite )
                {
                    _sequencer.Load ( fullFile );
                
                    JsonTree t { loadFile ( fullFile ) };
                    if ( t.hasChild ( "Flow.Alpha" ) ) _flowField->Alpha = t["Flow.Alpha"];
                    if ( t.hasChild ( "Particles.Alpha" ) ) _particles.Alpha = t["Particles.Alpha"];
                    if ( t.hasChild ( "Density.Alpha" ) ) _fluid->Alpha = t["Density.Alpha"];
                    if ( t.hasChild ( "Metalness.Alpha" ) ) _fluid->Metalness = t["Metalness.Alpha"];
                
                    kLastWrite = n;
                    OnReload    ( );
                }
            }
        }
        catch ( const std::exception& e )
        {
            std::cout << e.what() << std::endl;
        }
    }
#else

#endif
    
    _fluid->ObstaclesDirty = true;
    
    if ( _running )
    {
        float t = _sequencer.Time();

#ifndef STANDALONE_DEMO
        _sequencer.StepBy( 1.0 / 60.0f );
        ApplyEncoders ( );
        
        if ( _isLeft && _syncFrameInterval > 0 )
        {
            if ( ( app::getElapsedFrames() % ( _syncFrameInterval ) ) == 0 )
            {
                _syncTransport->SendEvent( "/sync", _sequencer.Time() );
            }
        }
        
        for ( auto& e : _sequencer.GetEmitters() )
        {
            _fluid->AddTemporalForce( { e->PositionAt(t), e->VelocityAt(t), e->ColorAt(t), e->RadiusAt(t), e->TemperatureAt(t), e->DensityAt(t) } );
        }
#endif

        _particles.DensityAlphaMultiplier = _fluid->Alpha.ValueAtTime(t);
        _particles.Update ( 1.0 / 60.0, _fluid->GetVelocity() );
        _fluid->Update ( 1.0 / 60.0 );
        
        BroadcastOSCChanges ( );
    }
}

void FluidApp::ApplyEncoders ( )
{
    if ( _encoders->IsConnected() )
    {
        float t = _sequencer.Time();
        for ( int i = 0; i < _encoderMappings.size(); i++ )
        {
            auto& options = _encoderMappings[i];
            for ( auto& o : options )
            {
                auto elem = _elementCache[o];
                if ( elem && elem->RadiusAt(t) > 0.01f )
                {
                    switch ( elem->GetType() )
                    {
                        case Time::ElementType::Emitter :
                        {
                            auto e = std::static_pointer_cast<Time::Emitter>( elem );
                            float a = _encoders->ValueAt ( i ) * M_PI * 2.0f;
                            e->Velocity().OverrideValue ( vec2 { std::cos( a ), std::sin( a ) } );
                            break;
                        }
                            
                        case Time::ElementType::Obstacle :
                        {
                            auto e = std::static_pointer_cast<Time::Obstacle>( elem );
                            e->Rotation().OverrideValue ( _encoders->ValueAt ( i ) * M_PI * 2.0f );
                            break;
                        }
                            
                        default : { }
                    }
                }
            }
        }
    }
}

void FluidApp::BroadcastOSCChanges ( )
{
    static float kLastSmokeValue    = -1.0f;
    static float kLastMetalValue    = -1.0f;
    static float kLastParticleValue = -1.0f;
    static float kLastFlowValue     = -1.0f;
    
    if ( _oscChannel )
    {
        float t = _sequencer.Time();
        
        float smoke     = _fluid->Alpha.ValueAtTime( t );
        float metal     = _fluid->Metalness.ValueAtTime( t );
        float particles = _particles.Alpha.ValueAtTime( t );
        float flow      = _flowField->Alpha.ValueAtTime( t );
        
        bool smokeChanged       = smoke     != kLastSmokeValue;
        bool metalChanged       = metal     != kLastMetalValue;
        bool particlesChanged   = particles != kLastParticleValue;
        bool flowChanged        = flow      != kLastFlowValue;
        
        if ( smokeChanged     ) _oscChannel->SendEvent ( kSmokeOSCAddress, smoke );
        if ( metalChanged     ) _oscChannel->SendEvent ( kMetalOSCAddress, metal );
        if ( particlesChanged ) _oscChannel->SendEvent ( kParticlesOSCAddress, particles );
        if ( flowChanged      ) _oscChannel->SendEvent ( kFlowOSCAddress, flow );
        
        kLastSmokeValue    = smoke;
        kLastMetalValue    = metal;
        kLastParticleValue = particles;
        kLastFlowValue     = flow;
    }
}

void FluidApp::OnReload ( )
{
    std::cout << "Reloading!\n";
    
    _errorList.clear();
    _elementCache.clear();
    
    for ( auto& e : _encoderMappings )
    {
        for ( auto& m : e )
        {
            auto elem = _sequencer.FindElement ( m );
            if ( elem )
            {
                _elementCache[m] = elem;
            }else
            {
                _errorList.push_back( "EncoderMapping: Error finding element '" + m + "' in Sequence" );
            }
        }
    }
    
    if ( _isLeft ) _syncTransport->SendEvent( "/sync", _sequencer.Time() );
}

void FluidApp::HandleKeyDown ( KeyEvent event )
{
    switch ( event.getCode() )
    {
        case KeyEvent::KEY_BACKQUOTE :
        {
            _uiEnabled = !_uiEnabled;
            if ( _uiEnabled )
            {
                showCursor();
            }else
            {
                hideCursor();
            }
            break;
        }
            
        case KeyEvent::KEY_s :
        {
            _fluid->LoadShaders();
            break;
        }
            
        case KeyEvent::KEY_c :
        {
            _fluid->Clear();
            break;
        }
        
        case KeyEvent::KEY_r :
        {
            _running = !_running;
            break;
        }
    }
}

void FluidApp::OnDraw ( )
{
    gl::clear ( Colorf::gray(0.0f) );
    RenderScene ( );
    
    if ( _uiEnabled ) RenderUI();
}

void FluidApp::RenderScene ( )
{
    gl::setMatricesWindow ( getWindowSize() );
    gl::enableAlphaBlending();
    
    {
        gl::ScopedColor color { ColorAf::white() };
        _fluid->Draw ( getWindowBounds() );
    }
    
    if ( true )
    {
        gl::ScopedDepth depth { false };
        gl::ScopedBlendAlpha blend;
        if ( _fluid->ObstacleRenderHandler )
        {
            gl::ScopedColor color { Colorf::black() };
            _fluid->ObstacleRenderHandler ( getWindowBounds(), true );
        }
    }
    
    _flowField->Draw ( );
    _particles.Draw( _fluid->GetDensity() );
    
    RenderOverlays ( );
    
    if ( kDrawBuffers ) _fluid->DrawBuffers();
}

void FluidApp::RenderOverlays ( )
{
    static ImageSequence kAttractorInSeq { getAssetPath ( "Animations/force_IN" ) };
    static ImageSequence kAttractorOutSeq { getAssetPath ( "Animations/force_Out" ) };
    
    float t = _sequencer.Time();
    
    if ( kDrawAttractors )
    {
        for ( auto& a : _sequencer.GetAttractors() )
        {
            auto& seq = a->ForceAt ( t ) > 0 ? kAttractorInSeq : kAttractorOutSeq;
            seq.DrawInRect( a->GetBoundsAt( t ) );
        }
    }
}

void FluidApp::RenderUI ( )
{
    {
        ui::ScopedWindow window { "Sequencer" };
        _sequencer.Inspect();
    }
    
#ifndef NDEBUG
    static const char * kConfiguration = "Debug";
#else
    static const char * kConfiguration = "Release";
#endif
    
    static const char * kVersion = (const char *)glGetString(GL_VERSION);
    static const char * kRenderer = (const char *)glGetString(GL_RENDERER);
    
    ui::ScopedWindow window { "Settings" };
    ui::Text ( "Scienceworks Fluid Simulator (Production)" );
    ui::Text ( "Build Configuration: %s", kConfiguration );
    ui::Text ( "OpenGL Version: %s", kVersion );
    ui::Text ( "OpenGL Renderer: %s", kRenderer );
    ui::Text ( "Layout: %s", _isLeft ? "Left" : "Right" );
    if ( _oscChannel )
    {
        ui::Text ( "Audio Endpoint: %s:%d", _oscChannel->Endpoint.c_str(), _oscChannel->Port );
    }else
    {
        ui::Text ( "%s", "Invalid Audio Endpoint Supplied" );
    }
    
    if ( !_errorList.empty() )
    {
        if ( ui::CollapsingHeader ( "Errors" ) )
        {
            for ( auto& e : _errorList )
            {
                ui::TextColored( ImVec4(0.7, 0, 0, 1), "%s", e.c_str() );
            }
        }
    }
    
    ui::Text ( "FPS: %.2f", getAverageFps() );
    if ( ui::Button ( "Quit" ) ) quit();
    ui::Dummy( ImVec2(0, 10) );
    ui::Checkbox ( "Simulate", &_running );
    
    if ( !_running )
    {
        ui::SameLine();
        if ( ui::Button ( "Step" ) )
        {
            _fluid->Update ( 1.0 / 60.0f );
            _particles.Update ( 1.0 / 60.0, _fluid->GetVelocity() );
        }
    }

    if ( ui::CollapsingHeader ( "Fluid Simulation" ) )
    {
        ui::Indent();
        
        ui::TextUnformatted( "Adjust this slider if your framerate is unacceptable" );
        if ( ui::DragFloat( "Simulation Scale", &kScale, 0.01f, 0.05f, 0.6f ) )
        {
            if ( kScale > 0.0f )
            {
                InitFluidAtScale ( kScale );
            }
        }
        
        ui::Checkbox( "Draw Debug Buffers", &kDrawBuffers );
        ui::Checkbox( "Mouse Injects Forces", &kMousePaint );
        ui::Checkbox( "Draw Attractors", &kDrawAttractors );
        
        // @TODO(Andrew): flip y UV of line flow field
        _fluid->Inspect();
        _flowField->Inspect();
        
        ui::Unindent();
    }

    _particles.Inspect();
    _encoders->Inspect();
}

void FluidApp::OnCleanup ( )
{

}

#ifdef CINDER_MSW
    #ifndef COMPILING_NETWORKED_FLUID
        CINDER_APP( FluidApp, RendererGl ( RendererGl::Options().msaa ( 8 ) ), FluidApp::Init )
    #endif
#else
    CINDER_APP( FluidApp, RendererGl ( RendererGl::Options().msaa ( 8 ) ), FluidApp::Init )
#endif
