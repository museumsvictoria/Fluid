//
//  Force.h
//  Fluid
//
//  Created by Andrew Wright on 7/3/18.
//

#ifndef Fluid_Force_h
#define Fluid_Force_h

#include "Property.h"
#include "cinder/Json.h"

namespace Time
{
    enum class ElementType
    {
        Emitter,
        Attractor,
        Obstacle
    };
    
    using ElementRef                = std::shared_ptr<class Element>;
    using EmitterRef                = std::shared_ptr<class Emitter>;
    using AttractorRef              = std::shared_ptr<class Attractor>;
    using ObstacleRef               = std::shared_ptr<class Obstacle>;
    
    ElementRef                      MakeElement ( const ci::JsonTree& tree );
    
    class Element                   : public std::enable_shared_from_this<Element>
    {
    public:
        
        static bool                 kIsLeft;
        
        Element                     ( );
        virtual ~Element            ( ) { }
        
        inline ci::vec2             PositionAt          ( float t ) const { return _position.ValueAtTime ( _startTime + t ); }
        inline float                RadiusAt            ( float t ) const { return _radius.ValueAtTime ( _startTime + t ); }
        virtual ci::Rectf           GetBoundsAt         ( float t ) const;
        
        inline const Vec2Property&  Position            ( ) const { return _position; }
        inline Vec2Property&        Position            ( ) { return _position; }
        
        inline const FloatProperty& Radius              ( ) const { return _radius; }
        inline FloatProperty&       Radius              ( ) { return _radius; }
        
        inline float&               StartTime           ( ) { return _startTime; }
        inline std::string&         Name                ( ) { return _name; }
        
        virtual void                Serialize           ( ci::JsonTree& tree );
        virtual void                Marshal             ( const ci::JsonTree& tree );
        
        void                        Inspect             ( );
        virtual ElementType         GetType             ( ) const = 0;
    
    protected:
        
        virtual void                InternalInspect     ( );
        
        int                         _id{0};
        std::string                 _name;
        float                       _startTime{0.0f};
        
        Vec2Property                _position{ci::vec2(100.0f, 180.0f)};
        FloatProperty               _radius{64.0f};
    };
    
    ///
    /// Emitter
    ///
    
    class Emitter : public Element
    {
    public:
        
        static void                 Init                ( );
        
        ElementType                 GetType             ( ) const override { return ElementType::Emitter; };
        
        inline ci::vec2             VelocityAt          ( float t ) const { return _velocity.ValueAtTime ( _startTime + t ); }
        inline ci::Colorf           ColorAt             ( float t ) const { return _color.ValueAtTime ( _startTime + t ); }
        inline float                TemperatureAt       ( float t ) const { return _temperature.ValueAtTime ( _startTime + t ); }
        inline float                DensityAt           ( float t ) const { return _density.ValueAtTime ( _startTime + t ); }
        
        inline const Vec2Property&  Velocity            ( ) const { return _velocity; }
        inline Vec2Property&        Velocity            ( ) { return _velocity; }
        
        inline const ColorProperty& Color               ( ) const { return _color; }
        inline ColorProperty&       Color               ( ) { return _color; }
        
        inline const FloatProperty& Temperature         ( ) const { return _temperature; }
        inline FloatProperty&       Temperature         ( ) { return _temperature; }
        
        inline const FloatProperty& Density             ( ) const { return _density; }
        inline FloatProperty&       Density             ( ) { return _density; }
        
        void                        Serialize           ( ci::JsonTree& tree ) override;
        void                        Marshal             ( const ci::JsonTree& tree ) override;
        
    protected:
        
        void                        InternalInspect     ( ) override;
        
        Vec2Property                _velocity{ci::vec2(1.0f, 0.0f)};
        ColorProperty               _color{ci::Colorf(0.5f, 0.10000000149011612, 0.0099999997764825821)};
        FloatProperty               _temperature{10.0f};
        FloatProperty               _density{1.0f};
    };
    
    ///
    /// Attractor
    ///
    
    class Attractor : public Element
    {
    public:
        
        static void                 Init                ( );
        
        ElementType                 GetType             ( ) const override { return ElementType::Attractor; };
        
        inline float                ForceAt             ( float t ) const { return _force.ValueAtTime ( _startTime + t ); }

        inline const FloatProperty& Force               ( ) const { return _force; }
        inline FloatProperty&       Force               ( ) { return _force; }
        
        void                        Serialize           ( ci::JsonTree& tree ) override;
        void                        Marshal             ( const ci::JsonTree& tree ) override;
        
    protected:
        
        void                        InternalInspect     ( ) override;
        
        FloatProperty               _force{0.5f};
    };
    
    ///
    /// Obstacle
    ///
    
    class Obstacle : public Element
    {
    public:
        
        static void                 Init                ( );
        static ci::gl::TextureRef   TextureAt           ( int textureIndex );
        
        ElementType                 GetType             ( ) const override { return ElementType::Obstacle; };
        
        inline float                RotationAt          ( float t ) const { return _rotation.ValueAtTime ( _startTime + t ); }
        
        inline const FloatProperty& Rotation            ( ) const { return _rotation; }
        inline FloatProperty&       Rotation            ( ) { return _rotation; }
        
        void                        TextureIndex        ( int t ) { _textureIndex = t; };
        int                         TextureIndex        ( ) const { return _textureIndex; };
        
        void                        Serialize           ( ci::JsonTree& tree ) override;
        void                        Marshal             ( const ci::JsonTree& tree ) override;
        
        void                        Draw                ( float overhang = 1.0f );
        
    protected:
        
        void                        InternalInspect     ( ) override;
        
        FloatProperty               _rotation{0.0f};
        int                         _textureIndex{0};
    };
}

#endif /* Fluid_Force_h */
