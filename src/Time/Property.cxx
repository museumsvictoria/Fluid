//
//  Property.cxx
//  Fluid
//
//  Created by Andrew Wright on 7/3/18.
//

#include <Time/Property.h>
#include "CinderImGui.h"

using namespace ci;

namespace Time
{
    ///
    /// Interpolation
    ///
    
    static std::vector<std::string> kEaseFnNames =
    {
        "EaseNone",
        "EaseInQuad", "EaseOutQuad", "EaseInOutQuad", "EaseOutInQuad",
        "EaseInCubic", "EaseOutCubic", "EaseInOutCubic", "EaseOutInCubic",
        "EaseInQuad", "EaseOutQuad", "EaseInOutQuad", "EaseOutInQuad",
        "EaseInQuart", "EaseOutQuart", "EaseInOutQuart", "EaseOutInQuart",
        "EaseInQuint", "EaseOutQuint", "EaseInOutQuint", "EaseOutInQuint",
        "EaseInSine", "EaseOutSine", "EaseInOutSine", "EaseOutInSine",
        "EaseInExpo", "EaseOutExpo", "EaseInOutExpo", "EaseOutInExpo",
        "EaseInCirc", "EaseOutCirc", "EaseInOutCirc", "EaseOutInCirc",
        "EaseInBounce", "EaseOutBounce", "EaseInOutBounce", "EaseOutInBounce",
        "EaseInBack", "EaseOutBack", "EaseInOutBack", "EaseOutInBack"
    };
    
    static std::vector<EaseFn> kEaseFns =
    {
        ci::EaseNone{},
        ci::EaseInQuad{},  ci::EaseOutQuad{}, ci::EaseInOutQuad{}, ci::EaseOutInQuad{},
        ci::EaseInCubic{}, ci::EaseOutCubic{}, ci::EaseInOutCubic{}, ci::EaseOutInCubic{},
        ci::EaseInQuad{}, ci::EaseOutQuad{}, ci::EaseInOutQuad{}, ci::EaseOutInQuad{},
        ci::EaseInQuart{}, ci::EaseOutQuart{}, ci::EaseInOutQuart{}, ci::EaseOutInQuart{},
        ci::EaseInQuint{}, ci::EaseOutQuint{}, ci::EaseInOutQuint{}, ci::EaseOutInQuint{},
        ci::EaseInSine{}, ci::EaseOutSine{}, ci::EaseInOutSine{}, ci::EaseOutInSine{},
        ci::EaseInExpo{}, ci::EaseOutExpo{}, ci::EaseInOutExpo{}, ci::EaseOutInExpo{},
        ci::EaseInCirc{}, ci::EaseOutCirc{}, ci::EaseInOutCirc{},ci::EaseOutInCirc{},
        ci::EaseInBounce{}, ci::EaseOutBounce{}, ci::EaseInOutBounce{}, ci::EaseOutInBounce{},
        ci::EaseInBack{}, ci::EaseOutBack{}, ci::EaseInOutBack{}, ci::EaseOutInBack{},
    };
    
    float ApplyEase ( int easeFnIndex, float percent )
    {
        return kEaseFns[easeFnIndex] ( percent );
    }
    
    ///
    /// Inspection
    ///

    template <typename T>
    void DoInspect ( const std::string& name, PropertyT<T>& property,  const std::function<void(T&)>& work )
    {
        if ( ui::TreeNode ( name.c_str() ) )
        {
            int toDelete = -1;
            ui::Indent();
            for ( int i = 0; i < property.Keyframes().size(); i++ )
            {
                ui::ScopedId id { i };
                auto& k = property.Keyframes()[i];
                auto n = "[" + std::to_string(i) + "]";
                bool tree = ui::TreeNode( n.c_str() );
                
                if ( i > 0 || property.Keyframes().size() > 1 )
                {
                    if ( ui::BeginPopupContextItem( "POP" ) )
                    {
                        if ( ui::Button( "Delete Keyframe" ) )
                        {
                            toDelete = i;
                        }
                        ui::EndPopup();
                    }
                }
                
                if ( tree )
                {
                    if ( property.Keyframes().size() > 1 )
                    {
                        ui::Combo( "Ease Curve", &k.EaseFnIndex, kEaseFnNames );
                        ui::DragFloat( "Time", &k.Time, 0.01f, 0.0f, 1024.0f );
                    }
                    work ( k.Value );
                    ui::TreePop();
                }
            }
            
            if ( toDelete > -1 )
            {
                property.Keyframes().erase ( property.Keyframes().begin() + toDelete );
            }
            
            if ( ui::Button ( ( "+" ) ) )
            {
                property.AddKeyFrame();
            }
            
            ui::Unindent();
            ui::TreePop();
        }
    }
    
    void Inspect ( ColorProperty& property, const std::string& name, uint32_t flags )
    {
        DoInspect<Colorf>( name, property, [] ( Colorf& v )
        {
            ui::ColorEdit3( "Value", &v.r );
        } );
    }
    
    void Inspect ( ColorAProperty& property, const std::string& name, uint32_t flags )
    {
        DoInspect<ColorAf>( name, property, [] ( ColorAf& v )
        {
            ui::ColorEdit4( "Value", &v.r );
        } );
    }
    
    void Inspect ( Vec2Property& property, const std::string& name, uint32_t flags )
    {
        DoInspect<vec2>( name, property, [flags] ( vec2& v )
        {
            if ( flags & BetweenZeroAndOne )
            {
                ui::DragFloat2( "Value", &v.x, 0.01f, 0.0f, 1.0f );
            }else
            {
                ui::DragFloat2( "Value", &v.x, 0.1f );
            }
        } );
    }
    
    void Inspect ( Vec3Property& property, const std::string& name, uint32_t flags )
    {
        DoInspect<vec3>( name, property, [flags] ( vec3& v )
        {
            if ( flags & BetweenZeroAndOne )
            {
                ui::DragFloat3( "Value", &v.x, 0.01f, 0.0f, 1.0f );
            }else
            {
                ui::DragFloat3( "Value", &v.x, 0.1f );
            }
        } );
    }
    
    void Inspect ( FloatProperty& property, const std::string& name, uint32_t flags )
    {
        DoInspect<float>( name, property, [flags] ( float& v )
        {
            if ( flags & IsAngle )
            {
                ui::SliderAngle( "Value", &v, 0.1f );
            }else
            {
                if ( flags & BetweenZeroAndOne )
                {
                    ui::DragFloat( "Value", &v, 0.01f, 0.0f, 1.0f );
                }else
                {
                    ui::DragFloat( "Value", &v, 0.1f );
                }
            }
        } );
    }
    
    void Inspect ( IntProperty& property, const std::string& name, uint32_t flags )
    {
        DoInspect<int>( name, property, [] ( int& v )
        {
            ui::DragInt( "Value", &v );
        } );
    }
    
    ///
    /// Serialization
    ///
    
    void SerializeKeyframe ( JsonTree& tree, const Keyframe& k )
    {
        tree.pushBack( JsonTree ( "Time", k.Time ) );
        tree.pushBack( JsonTree ( "EaseFnIndex", k.EaseFnIndex ) );
    }
    
    template <>
    void Serialize ( JsonTree& tree, const KeyframeT<Colorf>& property )
    {
        SerializeKeyframe( tree, property );
        
        JsonTree v = JsonTree::makeObject("Value");
        v.pushBack( JsonTree ( "R", property.Value.r ) );
        v.pushBack( JsonTree ( "G", property.Value.g ) );
        v.pushBack( JsonTree ( "B", property.Value.b ) );
        tree.pushBack( v );
    }
    
    template <>
    void Marshal ( const JsonTree& tree, Colorf& result )
    {
        result.r = tree["R"].getValue<float>();
        result.g = tree["G"].getValue<float>();
        result.b = tree["B"].getValue<float>();
    }
    
    template <>
    void Serialize ( JsonTree& tree, const KeyframeT<ColorAf>& property )
    {
        SerializeKeyframe( tree, property );
        JsonTree v = JsonTree::makeObject("Value");
        v.pushBack( JsonTree ( "R", property.Value.r ) );
        v.pushBack( JsonTree ( "G", property.Value.g ) );
        v.pushBack( JsonTree ( "B", property.Value.b ) );
        v.pushBack( JsonTree ( "A", property.Value.a ) );
        tree.pushBack( v );
    }
    
    template <>
    void Marshal ( const JsonTree& tree, ColorAf& result )
    {
        result.r = tree["R"].getValue<float>();
        result.g = tree["G"].getValue<float>();
        result.b = tree["B"].getValue<float>();
        result.a = tree["A"].getValue<float>();
    }
    
    template <>
    void Serialize ( JsonTree& tree, const KeyframeT<vec2>& property )
    {
        SerializeKeyframe( tree, property );
        JsonTree v = JsonTree::makeObject("Value");
        v.pushBack( JsonTree ( "X", property.Value.x ) );
        v.pushBack( JsonTree ( "Y", property.Value.y ) );
        tree.pushBack( v );
    }
    
    template <>
    void Marshal ( const JsonTree& tree, vec2& result )
    {
        result.x = tree["X"].getValue<float>();
        result.y = tree["Y"].getValue<float>();
    }
    
    template <>
    void Serialize ( JsonTree& tree, const KeyframeT<vec3>& property )
    {
        SerializeKeyframe( tree, property );
        JsonTree v = JsonTree::makeObject("Value");
        v.pushBack( JsonTree ( "X", property.Value.x ) );
        v.pushBack( JsonTree ( "Y", property.Value.y ) );
        v.pushBack( JsonTree ( "Z", property.Value.z ) );
        tree.pushBack( v );
    }
    
    template <>
    void Marshal ( const JsonTree& tree, vec3& result )
    {
        result.x = tree["X"].getValue<float>();
        result.y = tree["Y"].getValue<float>();
        result.z = tree["Z"].getValue<float>();
    }
    
    template <>
    void Serialize ( JsonTree& tree, const KeyframeT<float>& property )
    {
        SerializeKeyframe( tree, property );
        tree.pushBack( JsonTree ( "Value", property.Value ) );
    }
    
    template <>
    void Marshal ( const JsonTree& tree, float& result )
    {
        result = tree.getValue<float>();
    }
    
    template <>
    void Serialize ( JsonTree& tree, const KeyframeT<int>& property )
    {
        SerializeKeyframe( tree, property );
        tree.pushBack( JsonTree ( "Value", property.Value ) );
    }
    
    template <>
    void Marshal ( const JsonTree& tree, int& result )
    {
        result = tree.getValue<int>();
    }
}
