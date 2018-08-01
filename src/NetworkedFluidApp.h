//
//  NetworkedFluidApp.h
//  Fluid
//
//  Created by Andrew Wright on 29/6/17.
//
//

#ifndef NetworkedFluidApp_h
#define NetworkedFluidApp_h

#include "cinder/app/App.h"
#include "Fluid.h"
#include "ParticleSystem.h"
#include "FlowField.h"
#include "WebSocketClient.h"

class NetworkedFluidApp : public ci::app::App
{
public:
    
    enum class Command : uint8_t
    {
        Join        = 0,
        Update      = 1,
        Leave       = 2,
        SetProperty = 3,
    };
    
    enum class Property : uint8_t
    {
        FluidAlpha,
        Metalness,
        FlowFieldAlpha,
        FlowFieldWeight,
        ParticleAlpha,
    };
    
    struct User
    {
        int                         ID{0};
        std::string                 Name;
        ci::vec2                    Position;
        ci::ColorAf                 Color;
        float                       Angle{0.0f};
        float                       Radius{64.0f};
        double                      Timestamp{0.0};
        
        void                        Unpack              ( const ci::IStreamMemRef& stream );
        bool                        Dead                ( ) const;
    };
    
    NetworkedFluidApp               ( );
    
    static void                     Init                ( ci::app::App::Settings * settings );
    
    void                            OnSetup             ( );
    void                            OnUpdate            ( );
    void                            OnDraw              ( );
    void                            OnCleanup           ( );
    
protected:
    
    void                            RenderScene         ( );
    void                            RenderUI            ( );
    
    Fluid::SimRef                   _fluid;
    ParticleSystem                  _particles;
    FlowFieldRef                    _flowField;
    
    std::unordered_map<int, User>   _users;
    WebSocketClient                 _client;
    bool                            _isConnected{false};
    std::string                     _endpoint;
};

#endif /* NetworkedFluidApp_h */
