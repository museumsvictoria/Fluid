//
//  NetworkedFluidApp.cxx
//  Fluid
//
//  Created by Andrew Wright on 29/6/17.
//
//

#include "NetworkedFluidApp.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "CinderImGui.h"

using namespace ci;
using namespace ci::app;
using namespace std;

#define RUN_WINDOWED

///
/// User

void NetworkedFluidApp::User::Unpack ( const IStreamMemRef& stream )
{
    Timestamp = app::getElapsedSeconds();
    
    stream->read( &Position.x ); //
    stream->read( &Position.y ); //
    stream->read( &Angle );
    stream->read( &Radius );
    
    uint8_t rgba[4];
    stream->readData ( &rgba, 4 );
    
    rgba[0] = std::max ( (uint8_t)25, rgba[0] );
    rgba[1] = std::max ( (uint8_t)25, rgba[1] );
    rgba[2] = std::max ( (uint8_t)25, rgba[2] );
    
    Color = ColorA8u ( rgba[0], rgba[1], rgba[2], rgba[3] );
    Radius *= 64.0f;
}

bool NetworkedFluidApp::User::Dead ( ) const
{
    return ( app::getElapsedSeconds() - Timestamp ) > 10.0f;
}

///
/// NetworkedFluidApp
///

static float kScale = 0.25f;

NetworkedFluidApp::NetworkedFluidApp ( )
{
#ifdef CINDER_MAC
    #ifndef RUN_WINDOWED
        getWindow()->spanAllDisplays();
    #endif
#endif
    OnSetup ( );
    
    getSignalUpdate().connect ( std::bind ( &NetworkedFluidApp::OnUpdate, this ) );
    getWindow()->getSignalDraw().connect ( std::bind ( &NetworkedFluidApp::OnDraw, this ) );
    getWindow()->getSignalKeyDown().connect( std::bind ( &NetworkedFluidApp::OnKeyDown, this, std::placeholders::_1 ) );
    getSignalCleanup().connect ( std::bind ( &NetworkedFluidApp::OnCleanup, this ) );
}

void NetworkedFluidApp::OnSetup ( )
{
    ui::initialize();
    
    JsonTree tree { loadAsset ( "Config.json" ) };
    if ( tree.hasChild( "WebSocketEndpoint" ) )
    {
        _endpoint = tree["WebSocketEndpoint"].getValue();
    }
    
    _tweak = Utils::QC( "Tweak.json", { { "LogoScale", &_logoScale },
                                        { "Gravity", &_gravity },
                                        { "ParticleAlpha", &_particleAlpha },
                                        { "FlowFieldAlpha", &_flowFieldAlpha },
                                        { "FlowFieldColorWeight", &_flowFieldColorWeight },
                                        { "FluidAlpha", &_fluidAlpha } } );
    
    _fluid = Fluid::Sim::Create( getWindowWidth(), getWindowHeight(), kScale );
    _fluid->Gravity.OverrideValue( vec2(0) );
    _fluid->Alpha.OverrideValue(0.09f);
    _fluid->Metalness.OverrideValue(0.0f);
    
    try
    {
        auto fmt = gl::Texture::Format().mipmap().minFilter(GL_LINEAR_MIPMAP_LINEAR).magFilter(GL_LINEAR);
        _logoTexture = gl::Texture::create ( loadImage( app::loadAsset( "BP-LOGO-BLACK.png" ) ), fmt );
        
        if ( _logoTexture )
        {
            _fluid->EnableObstacles(true);
            _fluid->ObstacleRenderHandler = [=] ( const Rectf& rect, bool topLeft )
            {
                if ( _fluid->AreObstaclesEnabled() && _logoScale > 0.0f )
                {
                    gl::ScopedMatrices m;
                    gl::setMatricesWindow ( rect.getSize(), topLeft );
                    gl::scale ( vec2 ( rect.getWidth() / (float)getWindowWidth() ) );
                    
                    float overhang = topLeft ? 1.001f : 1.0f;
                    
                    Rectf b = _logoTexture->getBounds();
                    b += getWindowCenter();
                    b -= b.getSize() / 2.0f;
                    b.scaleCentered( _logoScale * overhang );
                    
                    gl::draw ( _logoTexture, b );
                }
            };
        }
            
    }catch ( const std::exception& e )
    {
            
    }
    
    _particles.Init( 9 );
    _particles.Scale = kScale;
    _particles.Alpha.OverrideValue(1.0f);
    _particles.AlphaMin = 0.0f;
    _particles.AlphaMultiplier = 2.0f;
    _particles.VelocityMultiplier = 0.006f;
    
    _flowField = std::make_unique<FlowField>( _fluid.get() );
    _flowField->Alpha.OverrideValue(0.0f);
    _flowField->ColorWeight.OverrideValue(0.7f);
    
    _client.connectMessageEventHandler ( [&] ( const std::string& message )
    {
        IStreamMemRef stream = IStreamMem::create ( message.data(), message.length() );
        
        uint8_t sot;
        stream->read( &sot );
        
        uint32_t packetSize = 0;
        stream->read ( &packetSize );
        
        uint8_t command = 0;
        stream->read( &command );
        
        switch ( (Command)command )
        {
            case Command::Leave :
            {
                int id = 0;
                stream->read ( &id );
                
                if ( _users.count( id ) )
                {
                    _users.erase ( id );
                }
                
                break;
            }
                
            case Command::Join :
            {
                int id = 0;
                stream->read ( &id );
                
                char buffer[32] = {};
                stream->readData( buffer, 32 );
                
                _users[id].ID = id;
                _users[id].Name = buffer;
                _users[id].Timestamp = app::getElapsedSeconds();
                
                break;
            }
                
            case Command::Update :
            {
                uint8_t numPeople = 0;
                stream->read ( &numPeople );
                
                for ( int i = 0; i < numPeople; i++ )
                {
                    int id = 0;
                    stream->read ( &id );
                    
                    _users[id].ID = id;
                    _users[id].Unpack ( stream );
                }
                
                break;
            }
                
            case Command::SetProperty :
            {
                uint8_t property = 0;
                float value = 0.0f;
                
                stream->read ( &property );
                stream->read( &value );
                
                switch ( (Property)property )
                {
                    case Property::FluidAlpha :
                    {
                        _fluid->Alpha.OverrideValue(value);
                        break;
                    }
                        
                    case Property::FlowFieldAlpha :
                    {
                        _flowField->Alpha.OverrideValue(value);
                        break;
                    }
                        
                    case Property::FlowFieldWeight :
                    {
                        _flowField->ColorWeight.OverrideValue(value);
                        break;
                    }
                        
                    case Property::ParticleAlpha :
                    {
                        _particles.Alpha.OverrideValue(value);
                        break;
                    }
                        
                    case Property::Metalness :
                    {
                        _fluid->Metalness.OverrideValue(value);
                        break;
                    }
                }
            }
        }
        
    } );
    
    _client.connectCloseEventHandler( [&] { _isConnected = false; });
    _client.connectOpenEventHandler( [&] { _isConnected = true; } );
}

void NetworkedFluidApp::OnUpdate ( )
{
    if ( !_isConnected && getElapsedFrames() % 60 == 0 )
    {
        if ( !_endpoint.empty() )
        {
            _client.connect( _endpoint );
        }
    }
    
    const float dt = 1.0 / 60.0f;
    
    auto it = _users.begin();
    while ( it != _users.end() )
    {
        if ( it->second.Dead() )
        {
            it = _users.erase( it );
        }else
        {
            auto& user = it->second;
            Fluid::Force force;
            force.Position = user.Position * vec2 ( getWindowSize() );
            force.Color = user.Color;
            force.Radius = user.Radius;
            force.Density = 0.8f;
            force.Velocity = vec2 { std::cos ( user.Angle ), std::sin ( user.Angle ) };
            _fluid->AddTemporalForce( force );
            it++;
            
            user.Radius *= 0.999f;
        }
    }
    
    _fluid->ObstaclesDirty = true;
    
    _fluid->Update( dt );
    _particles.Update( dt, _fluid->GetVelocity() );
    _client.poll();
}

void NetworkedFluidApp::OnKeyDown ( const app::KeyEvent& event )
{
    if ( event.getChar() == '`' )
    {
        _renderTweak = !_renderTweak;
    }
}

void NetworkedFluidApp::OnDraw ( )
{
    RenderScene ( );
    RenderUI ( );
    
}

void NetworkedFluidApp::RenderScene ( )
{
    gl::clear ( Colorf::black() );
    
    _fluid->Draw( getWindowBounds() );
    
    if ( _logoTexture && _logoScale > 0.0f )
    {
        gl::ScopedDepth depth { false };
        gl::ScopedBlendAlpha blend;
        if ( false && _fluid->ObstacleRenderHandler )
        {
            //gl::ScopedColor color { Colorf::black() };
            gl::ScopedColor color { Colorf(1, 0, 0) };
            _fluid->ObstacleRenderHandler ( getWindowBounds(), true );
        }
    }
    
    _particles.Draw( _fluid->GetDensity() );
    _flowField->Draw();
}

void NetworkedFluidApp::RenderUI ( )
{
    if ( !_isConnected )
    {
        gl::ScopedBlendAlpha blend;
        gl::ScopedColor c { ColorAf ( 1, 0, 0, std::sin ( getElapsedSeconds() * 3.0f ) * 0.5f + 0.5f ) };
        gl::drawSolidRect( Rectf ( 0, 0, 8, 8 ) );
    }
    
    if ( _endpoint.empty() )
    {
        gl::ScopedBlendAlpha blend;
        gl::ScopedColor c { ColorAf ( 1, 1, 0, std::sin ( getElapsedSeconds() * 3.0f ) * 0.5f + 0.5f ) };
        gl::drawSolidRect( Rectf ( 0, 0, 8, 8 ) );
    }
    
    static bool kFirst = true;
    
    if ( _renderTweak || kFirst ) // Make sure to commit the changes the first time around
    {
        ui::ScopedWindow window { "Tweak Settings" };
        
        //if ( kFirst || ui::DragFloat( "Logo Scale", &_logoScale, 0.001f, 0.0f, 3.0f ) ) { };
        if ( kFirst || ui::DragFloat2( "Gravity", &_gravity.x, 0.001f, -2.0f, 2.0f ) ) _fluid->Gravity.OverrideValue( _gravity );
        if ( kFirst || ui::DragFloat( "Particle Alpha", &_particleAlpha, 0.001f, 0.0f, 1.0f ) ) _particles.Alpha.OverrideValue( _particleAlpha );
        if ( kFirst || ui::DragFloat( "Flow Field Alpha", &_flowFieldAlpha, 0.001f, 0.0f, 1.0f ) ) _flowField->Alpha.OverrideValue( _flowFieldAlpha );
        if ( kFirst || ui::DragFloat( "Flow Field Color Weight", &_flowFieldColorWeight, 0.001f, 0.0f, 1.0f ) ) _flowField->ColorWeight.OverrideValue( _flowFieldColorWeight );
        if ( kFirst || ui::DragFloat( "Fluid Alpha", &_fluidAlpha, 0.001f, 0.0f, 1.0f ) ) _fluid->Alpha.OverrideValue( _fluidAlpha );
        
        if ( ui::Button( "Save" ) ) _tweak.Save();
        
        kFirst = false;
        
        _fluid->DrawBuffers();
    }
}

void NetworkedFluidApp::OnCleanup ( )
{
    _client.disconnect();
}

void Init ( App::Settings * settings )
{
#if 1
    settings->setFullScreen();
#else
    #ifndef RUN_WINDOWED
        settings->setAlwaysOnTop();
        settings->setBorderless();
    #else
        settings->setWindowSize(1280, 720);
    #endif
#endif
}

#ifdef CINDER_MSW
    #ifdef COMPILING_NETWORKED_FLUID
        CINDER_APP( NetworkedFluidApp, RendererGl ( RendererGl::Options().msaa(8) ), Init )
    #endif
#else
    CINDER_APP( NetworkedFluidApp, RendererGl ( RendererGl::Options().msaa(8) ), Init )
#endif
