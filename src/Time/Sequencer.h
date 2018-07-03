//
//  Sequencer.h
//  Fluid
//
//  Created by Andrew Wright on 20/3/18.
//

#ifndef Fluid_Sequencer_h
#define Fluid_Sequencer_h

#include "Force.h"
#include <Time/OSCChannel.h>

namespace Time
{
    class Sequencer : public ci::Noncopyable
    {
    public:
        
        static Sequencer&                   Default         ( );
        
        void                                StepTo          ( float time );
        void                                StepBy          ( float delta );
        
        inline float                        Time            ( ) const { return _time; }
        
        bool                                Load            ( const ci::fs::path& path );
        bool                                Save            ( const ci::fs::path& path );
        
        std::vector<EmitterRef>             GetEmitters     ( ) const;
        std::vector<AttractorRef>           GetAttractors   ( ) const;
        std::vector<ObstacleRef>            GetObstacles    ( ) const;
        
        const std::vector<ElementRef>&      GetElements     ( ) const { return _elements; }
        std::vector<ElementRef>&            GetElements     ( ) { return _elements; }
        
        void                                AddElement      ( const ElementRef& element );
        ElementRef                          CreateElement   ( ElementType type );
        ElementRef                          FindElement     ( const std::string& name );
        
        void                                Inspect         ( );
        void                                OnLoop          ( std::function<void()> handler ) { _loopHandler = handler; }
        
        float                               Duration{20.0f};
        
    protected:
        
        void                                OnLoop          ( );
        void                                FireEvents      ( );
        
        struct Event
        {
            bool                            Fired{false};
            float                           Time{0.0f};
            std::string                     Name;
            
            Event                           ( ) { };
            Event                           ( const ci::JsonTree& tree );
        };
        
        Sequencer                           ( );
        
        std::vector<Event>                  _events;
        std::vector<ElementRef>             _elements;
        float                               _time{0.0f};
        OSCChannelRef                       _oscChannel;
        std::function<void()>               _loopHandler;
    };
}

#endif /* Fluid_Sequencer_h */
