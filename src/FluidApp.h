//
//  FluidApp.h
//  Fluid
//
//  Created by Andrew Wright on 29/6/17.
//
//

#ifndef FluidApp_h
#define FluidApp_h

#include "cinder/app/App.h"
#include "cinder/Timeline.h"
#include "Fluid.h"
#include "ParticleSystem.h"
#include "FlowField.h"
#include "Time/Sequencer.h"
#include "Time/OSCChannel.h"
#include "RotaryEncoders.h"

class FluidApp : public ci::app::App
{
public:
    
    FluidApp                    ( );
    
    static void                 Init                ( ci::app::App::Settings * settings );
    
    void                        OnSetup             ( );
    void                        OnUpdate            ( );
    void                        OnDraw              ( );
    void                        OnCleanup           ( );
    
protected:
    
    using ElementCache          = std::unordered_map<std::string, Time::ElementRef>;
    using EncoderMapping        = std::vector<std::vector<std::string>>;
    
    void                        InitFluidAtScale    ( float scale = 0.5f );
    
    void                        HandleKeyDown       ( ci::app::KeyEvent event );
    
    void                        OnReload            ( );
    
    void                        RenderScene         ( );
    void                        RenderOverlays      ( );
    void                        RenderUI            ( );
    
    void                        ApplyEncoders       ( );
    void                        BroadcastOSCChanges ( );
    
    Fluid::SimRef               _fluid;
    ParticleSystem              _particles;
    FlowFieldRef                _flowField;
    RotaryEncodersRef           _encoders;
    
    Time::Sequencer&            _sequencer;
    Time::OSCChannelRef         _oscChannel;
    Time::OSCChannelRef         _syncTransport;
    bool                        _isLeft{false};
    int                         _edgeInset{0};
    int                         _syncFrameInterval{0};
    
    bool                        _uiEnabled{false};
    bool                        _running{true};
    
    ElementCache                _elementCache;
    EncoderMapping              _encoderMappings;
    std::vector<std::string>    _errorList;
};

#endif /* FluidApp_h */
