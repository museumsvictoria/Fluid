//
//  Sequencer.cxx
//  Fluid
//
//  Created by Andrew Wright on 20/3/18.
//

#include <Time/Sequencer.h>
#include "CinderImGui.h"

using namespace ci;

namespace Time
{
    Sequencer& Sequencer::Default ( )
    {
        static Sequencer kInstance;
        return kInstance;
    }
    
    void Sequencer::StepTo ( float time )
    {
        _time = time;
        if ( _time > Duration ) { _time -= Duration; OnLoop(); }
        if ( _time < 0.0f ) { _time += Duration; OnLoop(); }
        FireEvents();
    }
    
    void Sequencer::StepBy ( float delta )
    {
        _time += delta;
        if ( _time > Duration ) { _time -= Duration; OnLoop(); }
        if ( _time < 0.0f ) { _time += Duration; OnLoop(); }
        FireEvents();
    }
    
    void Sequencer::FireEvents ( )
    {
        for ( auto& e : _events )
        {
            if ( _time >= e.Time && !e.Fired )
            {
                if ( _oscChannel ) _oscChannel->SendEvent( e.Name );
                std::cout << "Firing " << e.Name << std::endl;
                e.Fired = true;
            }
        }
    }
    
    void Sequencer::OnLoop ( )
    {
        for ( auto& event : _events ) event.Fired = false;
        if ( _loopHandler ) _loopHandler();
    }
    
    void Sequencer::AddElement ( const ElementRef& element )
    {
        _elements.push_back ( element );
    }
    
    ElementRef Sequencer::CreateElement ( ElementType type )
    {
        ElementRef result = nullptr;
        
        switch ( type )
        {
            case ElementType::Emitter :
            {
                result = std::make_shared<Emitter>();
                break;
            }
                
            case ElementType::Attractor :
            {
                result = std::make_shared<Attractor>();
                break;
            }
                
            case ElementType::Obstacle :
            {
                result = std::make_shared<Obstacle>();
                break;
            }
        }
        
        if ( result ) AddElement( result );
        
        return result;
    }
    
    ElementRef Sequencer::FindElement ( const std::string& name )
    {
        for ( auto& e : GetElements() )
        {
            if ( e->Name() == name ) return e;
        }
        
        return nullptr;
    }
    
    Sequencer::Event::Event ( const JsonTree& tree )
    {
        Fired = false;
        Name  = tree["Event"].getValue<std::string>();
        Time  = tree["Time"].getValue<float>();
    }
    
    Sequencer::Sequencer ( )
    {
        Emitter::Init ( );
        Attractor::Init ( );
        Obstacle::Init ( );
    }
    
    bool Sequencer::Load ( const fs::path& path )
    {
        try
        {
            JsonTree tree { loadFile( path ) };
            
            float duration = tree["Duration"].getValue<float>();;
            std::vector<ElementRef> elements;
            
            for ( auto& e : tree["Elements" ] )
            {
                auto elem = MakeElement ( e );
                if ( elem ) elements.push_back( elem );
            }
            
            std::vector<Event> events;
            for ( auto& e : tree["Events"] )
            {
                events.emplace_back( e );
            }
            
            _elements = elements;
            _events = events;
            Duration = duration;
            
            return true;
        }catch ( const std::exception& e )
        {
            std::cout << "Error loading sequencer: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool Sequencer::Save ( const fs::path& path )
    {
        if ( path.empty() ) return false;
        
        JsonTree tree;
        tree.pushBack( JsonTree ( "Duration", Duration ) );
        
        JsonTree elements = JsonTree::makeArray("Elements");
        for ( auto& e : GetElements() )
        {
            JsonTree node = JsonTree::makeObject();
            e->Serialize( node );
            elements.pushBack( node );
        }
        
        tree.pushBack( elements );
        tree.write ( path );
        
        return true;
    }
    
    template <typename T>
    std::vector<std::shared_ptr<T>> Collect ( const std::vector<ElementRef>& elements, ElementType type )
    {
        std::vector<std::shared_ptr<T>> result;
        for ( auto& e : elements )
        {
            if ( e->GetType() == type ) result.push_back ( std::static_pointer_cast<T>( e ) );
        }
        
        return result;
    }
    
    std::vector<EmitterRef> Sequencer::GetEmitters ( ) const
    {
        return Collect<Emitter> ( _elements, ElementType::Emitter );
    }
    
    std::vector<AttractorRef> Sequencer::GetAttractors ( ) const
    {
        return Collect<Attractor> ( _elements, ElementType::Attractor );
    }
    
    std::vector<ObstacleRef> Sequencer::GetObstacles ( ) const
    {
        return Collect<Obstacle> ( _elements, ElementType::Obstacle );
    }
    
    void Sequencer::Inspect ( )
    {
        ui::ScopedId id { "Sequencer" };
        ui::Text ( "Time: %.2f", _time );
        for ( auto& e : _elements )
        {
            e->Inspect ( );
        }
        
        if ( ui::Button ( "+ Emitter" ) ) CreateElement( ElementType::Emitter );
        ui::SameLine();
        
        if ( ui::Button ( "+ Attractor" ) ) CreateElement( ElementType::Attractor );
        ui::SameLine();
        
        if ( ui::Button ( "+ Obstacle" ) ) CreateElement( ElementType::Obstacle );
        
        if ( ui::Button ( "Save" ) )
        {
            Save ( app::getSaveFilePath() );
        }
        
        ui::SameLine();
        
        if ( ui::Button ( "Load" ) )
        {
            Load ( app::getOpenFilePath() );
        }
    }
}
