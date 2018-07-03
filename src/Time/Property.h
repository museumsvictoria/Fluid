//
//  Property.h
//  Fluid
//
//  Created by Andrew Wright on 7/3/18.
//

#ifndef Fluid_Property_h
#define Fluid_Property_h

#include "cinder/Utilities.h"
#include "cinder/Timeline.h"
#include "cinder/Json.h"

namespace Time
{
    float                   ApplyEase ( int easeFnIndex, float percent );
    
    template <typename T>
    void                    Marshal   ( const ci::JsonTree& tree, T& resultValue );
    
    class Keyframe
    {
    public:
        Keyframe            ( ) { };
        Keyframe            ( const ci::JsonTree& tree )
        {
            Time            = tree["Time"].getValue<float>();
            EaseFnIndex     = tree["EaseFnIndex"].getValue<int>();
        }
        
        float               Time{0.0f};
        int                 EaseFnIndex{0};
    };
    
    template <typename T>
    class KeyframeT : public Keyframe
    {
    public:
        KeyframeT           ( ) { };
        KeyframeT           ( const ci::JsonTree& tree )
        : Keyframe ( tree )
        {
            Marshal         ( tree["Value"], Value );
        }
        
        T                   Value{T{}};
    };
    
    template <typename T>
    class PropertyT
    {
    public:
        PropertyT           ( T value = T() )
        {
            _keyframes.resize(1);
            _keyframes[0].Value = value;
        };
        
        PropertyT ( const ci::JsonTree& tree )
        {
            for ( auto& f : tree["Frames"] ) _keyframes.emplace_back ( f );
        }
        
        const std::vector<KeyframeT<T>>& Keyframes ( ) const { return _keyframes; }
        std::vector<KeyframeT<T>>& Keyframes       ( ) { return _keyframes; }
       
        inline const T&     ValueAtFrame          ( std::size_t frame ) const { return _keyframes[frame].Value; }
        inline T&           ValueAtFrame          ( std::size_t frame ) { return _keyframes[frame].Value; }
        
        inline const float& TimeAtFrame           ( std::size_t frame ) const { return _keyframes[frame].Time; };
        inline float&       TimeAtFrame           ( std::size_t frame ) { return _keyframes[frame].Time; }
        
        inline void         OverrideValue         ( const T& value )
        {
            for ( auto& f : _keyframes )
            {
                f.Value = value;
            }
        }
        
        inline void         Set                   ( std::size_t frame, float time, const T& value )
        {
            TimeAtFrame  ( frame ) = time;
            ValueAtFrame ( frame ) = value;
        }
        
        T                   ValueAtTime           ( float t ) const
        {
            if ( _keyframes.size() == 1 ) return ValueAtFrame ( 0 );
            
            if ( t < _keyframes.front().Time ) return _keyframes.front().Value;
            if ( t > _keyframes.back().Time ) return _keyframes.back().Value;
            
            for ( int i = 0; i < _keyframes.size() - 1; i++ )
            {
                auto& a = _keyframes[i+0];
                auto& b = _keyframes[i+1];
                
                if ( t >= a.Time && t <= b.Time )
                {
                    float percent = ci::lmap ( t, a.Time, b.Time, 0.0f, 1.0f );
                    return ci::lerp ( a.Value, b.Value, ApplyEase ( a.EaseFnIndex, percent ) );
                }
            }
            
            return _keyframes.back().Value;
        }
        
        void AddKeyFrame ( float t = -1.0f )
        {
            if ( !_keyframes.empty() )
            {
                auto& o = _keyframes.back();
                _keyframes.push_back({});
                _keyframes.back().Value = o.Value;
                _keyframes.back().Time = t == -1.0f ? o.Time + 2.0f : t;
            }else
            {
                _keyframes.push_back({});
                _keyframes.back().Time = std::max(t, 0.0f);
            }
        }
        
    protected:
        
        std::vector<KeyframeT<T>> _keyframes;
    };
    
    using ColorProperty     = PropertyT<ci::Colorf>;
    using ColorAProperty    = PropertyT<ci::ColorAf>;
    using Vec2Property      = PropertyT<ci::vec2>;
    using Vec3Property      = PropertyT<ci::vec3>;
    using FloatProperty     = PropertyT<float>;
    using IntProperty       = PropertyT<int>;
    
    template <typename T>
    PropertyT<T>            InitWithRange ( float t0, const T& v0, float t1, const T& v1 )
    {
        PropertyT<T> result;
        result.Keyframes()[0].Time  = t0;
        result.Keyframes()[0].Value = v0;
        
        result.Keyframes()[1].Time  = t1;
        result.Keyframes()[1].Value = v1;

        return result;
    }
    
    enum InspectFlags
    {
        IsAngle             = 0x01,
        BetweenZeroAndOne   = 0x02
    };
    
    void                    Inspect ( ColorProperty&  property, const std::string& name, uint32_t flags = 0 );
    void                    Inspect ( ColorAProperty& property, const std::string& name, uint32_t flags = 0 );
    void                    Inspect ( Vec2Property&   property, const std::string& name, uint32_t flags = 0 );
    void                    Inspect ( Vec3Property&   property, const std::string& name, uint32_t flags = 0 );
    void                    Inspect ( FloatProperty&  property, const std::string& name, uint32_t flags = 0 );
    void                    Inspect ( IntProperty&    property, const std::string& name, uint32_t flags = 0 );
    
    template <typename T>
    void                    Serialize ( ci::JsonTree& tree, const KeyframeT<T>& keyframe );
    
    template <typename T>
    void                    Serialize ( ci::JsonTree& tree, const PropertyT<T>& property, const std::string& name )
    {
        ci::JsonTree node = ci::JsonTree::makeObject(name);
        ci::JsonTree frames = ci::JsonTree::makeArray("Frames");
        
        for ( auto& f : property.Keyframes() )
        {
            ci::JsonTree node;
            Serialize ( node, f );
            frames.pushBack(node);
        }
        
        node.pushBack ( frames );
        tree.pushBack ( node );
    }
}

#endif /* Fluid_Property_h */
