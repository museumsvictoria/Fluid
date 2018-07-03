//
//  Fluid.h
//  Fluid
//
//  Created by Andrew Wright on 29/6/17.
//
//

#ifndef Fluid_Fluid_h
#define Fluid_Fluid_h

#include "PingPongBuffer.h"
#include "cinder/Json.h"
#include <Time/Force.h>
#include <Time/Sequencer.h>

#include <array>

namespace Fluid
{
    using SimRef = std::unique_ptr<class Sim>;
    
    struct Force
    {
        ci::vec2                    Position;
        ci::vec2                    Velocity;
        ci::Colorf                  Color;
        
        float                       Radius{1.0f};
        float                       Temperature{10.0f};
        float                       Density{1.0f};
        
        Force                       ( ) { }
        Force                       ( const ci::vec2& position, const ci::vec2& velocity, const ci::Colorf& color, float radius = 1.0f, float temperature = 10.0f, float density = 1.0f )
        : Position ( position )
        , Velocity ( velocity )
        , Color ( color )
        , Radius ( radius )
        , Temperature ( temperature )
        , Density ( density )
        { }
        
        Force                       ( const ci::JsonTree& tree, const ci::vec2& size = ci::vec2(1) );
        ci::JsonTree                ToJson ( const ci::vec2& size = ci::vec2(1) ) const;
            
    };
    
    struct ScopedFboDraw
    {
        ScopedFboDraw               ( const ci::gl::FboRef& buffer );
        ScopedFboDraw               ( const PingPongBuffer& buffer );
        ~ScopedFboDraw              ( );
        
    protected:
        
        ci::gl::FboRef              _buffer;
    };
    
    class Sim
    {
    public:
        
        using                       ObstacleRenderFn    = std::function<void(const ci::Rectf&, bool topLeft)>;
        
        static SimRef               Create              ( int width, int height, float scale = 0.5f );
        
        void                        Inspect             ( );
        void                        LoadShaders         ( );
        
        void                        EnableObstacles     ( bool enabled );
        inline bool                 AreObstaclesEnabled ( ) const { return _obstaclesEnabled; };
        
        void                        AddConstantForce    ( const Force& force );
        void                        AddTemporalForce    ( const Force& force );
        
        void                        Clear               ( float clearAlpha = 1.0f );
        void                        Update              ( double dt );
        
        void                        Draw                ( const ci::Rectf& bounds );
        void                        DrawBuffers         ( );
        void                        DrawVelocity        ( const ci::Rectf& bounds );
        
        ci::gl::TextureRef          GetVelocity         ( ) const { return _velocityBuffer->SourceTexture(); };
        ci::gl::TextureRef          GetDensity          ( ) const { return _densityBuffer->SourceTexture(); };
        ci::Surface8u               GetDensityEdge      ( float y0, float y1 ) const;
        
        inline float                Scale               ( ) const { return _scale; };
        inline const ci::ivec2&     Size                ( ) const { return _size; };
        
        std::vector<Force>&         ConstantForces      ( ) { return _constantForces; };
        
        float                       DensityDissipation;
        float                       VelocityDissipation;
        float                       TemperatureDissipation;
        float                       PressureDissipation;
        
        Time::Vec2Property          Gravity;
        Time::FloatProperty         SmokeBuoyancy{1.0f};
        Time::FloatProperty         SmokeWeight{0.05f};
        Time::FloatProperty         AmbientTemperature{0.0f};
        Time::FloatProperty         Alpha{1.0f};
        Time::FloatProperty         Metalness{0.0f};

        ObstacleRenderFn            ObstacleRenderHandler;
        bool                        ObstaclesDirty{false};
        
    protected:
        
        Sim                         ( int width, int height, float scale = 0.5f );
        
        void                        Advect              ( PingPongBuffer& buffer, float dissipation ) const;
        void                        Jacobi              ( ) const;
        void                        SubtractGradient    ( ) const;
        void                        ComputeDivergence   ( ) const;
        
        void                        ApplyForces         ( );
        void                        ApplyImpulse        ( PingPongBuffer& buffer, const ci::gl::TextureRef& texture, float weight = 1.0f, bool isVelocity = false );
        void                        ApplyImpulse        ( PingPongBuffer& buffer, const ci::vec3& force, const ci::vec3& value, float radius = 3.0f );
        void                        ApplyBuoyancy       ( );
        
        void                        UpdateAttractors    ( );
        
        void                        RenderQuad          ( int width, int height ) const;
        void                        ResetGLState        ( ) const;
        void                        ClearBuffer         ( const ci::gl::FboRef& buffer, const ci::ColorAf& clearColor = ci::ColorAf::black() );
        
        ci::gl::GlslProgRef         _advectShader;
        ci::gl::GlslProgRef         _jacobiShader;
        ci::gl::GlslProgRef         _subtractGradientShader;
        ci::gl::GlslProgRef         _computeDivergenceShader;
        ci::gl::GlslProgRef         _applyImpulseShader;
        ci::gl::GlslProgRef         _applyTextureShader;
        ci::gl::GlslProgRef         _applyBuoyancyShader;
        
        PingPongBufferRef           _velocityBuffer;
        PingPongBufferRef           _temperatureBuffer;
        PingPongBufferRef           _pressureBuffer;
        PingPongBufferRef           _densityBuffer;
        
        ci::gl::FboRef              _divergenceBuffer;
        ci::gl::FboRef              _obstacleBuffer;
        
        ci::gl::FboRef              _colorAddBuffer;
        ci::gl::FboRef              _velocityAddBuffer;
        
        std::vector<Force>          _constantForces;
        std::vector<Force>          _temporalForces;
        
        float                       _gradientScale{1.0f};
        float                       _gridWidth;
        float                       _gridHeight;
        float                       _timeStep;
        float                       _cellSize;
        
        ci::ivec2                   _size;
        float                       _scale{1.0f};
        
        int                         _numJacobiIterations{40};
        bool                        _obstaclesEnabled{true};
        
        Time::FloatProperty         _matCapPerturbation{0.3f};
        Time::FloatProperty         _matCapExponent{1.0f};
        ci::gl::TextureRef          _matCapTexture;
        ci::gl::GlslProgRef         _presentShader;
        Time::Sequencer&            _sequencer;
    };
}


#endif /* Fluid_Fluid_h */
