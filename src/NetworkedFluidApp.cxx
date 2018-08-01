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

using namespace ci;
using namespace ci::app;
using namespace std;

///
/// User

void NetworkedFluidApp::User::Unpack ( const IStreamMemRef& stream )
{
    Timestamp = app::getElapsedSeconds();
    
    stream->read( &Position.x );
    stream->read( &Position.y );
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
    getWindow()->spanAllDisplays();
#endif
    OnSetup ( );
    
    getSignalUpdate().connect ( std::bind ( &NetworkedFluidApp::OnUpdate, this ) );
    getWindow()->getSignalDraw().connect ( std::bind ( &NetworkedFluidApp::OnDraw, this ) );
    getSignalCleanup().connect ( std::bind ( &NetworkedFluidApp::OnCleanup, this ) );
}

void NetworkedFluidApp::OnSetup ( )
{
    JsonTree tree { loadAsset ( "Config.json" ) };
    if ( tree.hasChild( "WebSocketEndpoint" ) )
    {
        _endpoint = tree["WebSocketEndpoint"].getValue();
    }
    
    _fluid = Fluid::Sim::Create( getWindowWidth(), getWindowHeight(), kScale );
    _fluid->Gravity.OverrideValue( vec2(0) );
    _fluid->Alpha = 0.08f;
    _fluid->Metalness = 0.0;
    
    _particles.Init( 9 );
    _particles.Scale = kScale;
    _particles.Alpha.OverrideValue(1.0f);
    
    _flowField = std::make_unique<FlowField>( _fluid.get() );
    _flowField->Alpha.OverrideValue(1.0f);
    _flowField->ColorWeight.OverrideValue(0.6f);
    
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
            
            force.Velocity = vec2 { std::cos ( user.Angle ), std::sin ( user.Angle ) };
            _fluid->AddTemporalForce( force );
            it++;
            
            user.Radius *= 0.999f;
        }
    }
    
    _fluid->Update( dt );
    _particles.Update( dt, _fluid->GetVelocity() );
    _client.poll();
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
}

void NetworkedFluidApp::OnCleanup ( )
{
    _client.disconnect();
}

void Init ( App::Settings * settings )
{
    settings->setFullScreen();
}

#ifdef CINDER_MSW
    #ifdef COMPILING_NETWORKED_FLUID
        CINDER_APP( NetworkedFluidApp, RendererGl ( RendererGl::Options().msaa(8) ), Init )
    #endif
#else
    CINDER_APP( NetworkedFluidApp, RendererGl ( RendererGl::Options().msaa(8) ), Init )
#endif
