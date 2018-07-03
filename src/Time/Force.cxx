//
//  Force.cxx
//  Fluid
//
//  Created by Andrew Wright on 7/3/18.
//

#include <Time/Force.h>
#include <Time/Sequencer.h>
#include "CinderImGui.h"

using namespace ci;

namespace Time
{
    static int kNextID = 0;
    bool Element::kIsLeft = false;
   
    static const std::string& ElementTypeToString ( ElementType type )
    {
        static std::unordered_map<int, std::string> kElementTypeToString =
        {
            { (int)ElementType::Emitter, "Emitter" },
            { (int)ElementType::Attractor, "Attractor" },
            { (int)ElementType::Obstacle, "Obstacle" }
        };
        
        return kElementTypeToString[(int)type];
    }
    
    static ElementType ElementTypeFromString ( const std::string& name )
    {
        static std::unordered_map<std::string, ElementType> kNameToElementType =
        {
            { "Emitter", ElementType::Emitter },
            { "Attractor", ElementType::Attractor },
            { "Obstacle", ElementType::Obstacle }
        };
        
        return kNameToElementType[name];
    }
    
    ///
    /// Element
    ///
    
    ElementRef MakeElement ( const ci::JsonTree& tree )
    {
        auto type = ElementTypeFromString( tree["Type"].getValue() );
        switch ( type )
        {
            case ElementType::Emitter :
            {
                auto e = std::make_shared<Emitter>();
                e->Marshal( tree );
                return e;
            }
                
            case ElementType::Attractor :
            {
                auto e = std::make_shared<Attractor>();
                e->Marshal( tree );
                return e;
            }
                
            case ElementType::Obstacle :
            {
                auto e = std::make_shared<Obstacle>();
                e->Marshal( tree );
                return e;
            }
        }
        
        return nullptr;
    }
    
    Element::Element ( )
    : _id ( kNextID++ )
    , _name ( "Untitled Element" )
    { }
    
    void Element::Inspect ( )
    {
        ui::ScopedId id { _id };
        auto node = ui::TreeNode ( _name.c_str() );
        if ( ui::IsItemHovered() ) gl::drawStrokedCircle( PositionAt( Sequencer::Default().Time() ), 64.0f, 3.0f );
        if ( node )
        {
            InternalInspect();
            ui::TreePop();
        }
    }
    
    Rectf Element::GetBoundsAt ( float t ) const
    {
        vec2 p = PositionAt(t);
        float r = RadiusAt(t);
        return Rectf { p - vec2(r), p + vec2(r) };
    }
    
    void Element::Serialize ( JsonTree& tree )
    {
        tree.pushBack( JsonTree ( "Type", ElementTypeToString ( GetType() ) ) );
        tree.pushBack( JsonTree ( "Name", Name() ) );
        tree.pushBack( JsonTree ( "ID", _id ) );
        
        Time::Serialize( tree, _position, "Position" );
        Time::Serialize( tree, _radius,   "Radius" );
    }
    
    void Element::Marshal ( const ci::JsonTree& tree )
    {
        _name = tree["Name"].getValue();
        _id = tree["ID"].getValue<int>();
        
        _position = Vec2Property  ( tree["Position"] );
        _radius   = FloatProperty ( tree["Radius"] );
        
        kNextID = std::max(_id+1, kNextID);
    }
    
    void Element::InternalInspect ( )
    {
        Time::Inspect ( _position   , "Position"    );
        Time::Inspect ( _radius     , "Radius"      );
    }
    
    ///
    /// Emitter
    ///
    
    void Emitter::Init ( )
    {
        // Do global initialisation here
    }
    
    void Emitter::InternalInspect ( )
    {
        Element::InternalInspect ( );
        Time::Inspect ( _velocity   , "Velocity"    );
        Time::Inspect ( _color      , "Color"       );
        Time::Inspect ( _temperature, "Temperature" );
        Time::Inspect ( _density    , "Density", Time::BetweenZeroAndOne );
    }
    
    void Emitter::Serialize ( JsonTree& tree )
    {
        Element::Serialize( tree );
        
        Time::Serialize( tree, _velocity, "Velocity" );
        Time::Serialize( tree, _color, "Color" );
        Time::Serialize( tree, _temperature, "Temperature" );
        Time::Serialize( tree, _density, "Density" );
    }
    
    void Emitter::Marshal ( const ci::JsonTree& tree )
    {
        Element::Marshal( tree );
        
        _velocity    = Vec2Property  ( tree["Velocity"] );
        _color       = ColorProperty ( tree["Color" ] );
        _temperature = FloatProperty ( tree["Temperature" ] );
        _density     = FloatProperty ( tree["Density" ] );
    }
    
    ///
    /// Attractor
    ///
    
    void Attractor::Init ( )
    {
        // Do global initialisation here
    }
    
    void Attractor::InternalInspect ( )
    {
        Element::InternalInspect ( );
        Time::Inspect( _force, "Force" );
    }
    
    void Attractor::Serialize ( JsonTree& tree )
    {
        Element::Serialize( tree );
        Time::Serialize( tree, _force, "Force" );
    }
    
    void Attractor::Marshal ( const ci::JsonTree& tree )
    {
        Element::Marshal( tree );
        _force = FloatProperty ( tree["Force" ] );
    }
    
    ///
    /// Obstacle
    ///
    
    static std::vector<gl::TextureRef>  kObstacleTextures;
    static std::vector<std::string>     kObstacleTextureNames;
    
    void Obstacle::Init ( )
    {
        static bool kInit = false;
        if ( !kInit )
        {
            fs::directory_iterator it { app::getAssetPath( Element::kIsLeft ? "ObstaclesLeft" : "ObstaclesRight" ) }, end;
            while ( it != end )
            {
                if ( it->path().extension().string() == ".png" )
                {
                    auto fmt = gl::Texture::Format().mipmap().minFilter(GL_LINEAR_MIPMAP_LINEAR).magFilter(GL_LINEAR);
                    
                    kObstacleTextureNames.push_back( it->path().stem().string() );
                    kObstacleTextures.push_back ( gl::Texture::create( loadImage ( loadFile ( it->path() ) ), fmt ) );
                }
                it++;
            }
            
            kInit = true;
        }
    }
    
    gl::TextureRef Obstacle::TextureAt ( int textureIndex )
    {
        if ( textureIndex < 0 ) textureIndex = 0;
        if ( textureIndex > kObstacleTextures.size() - 1 ) textureIndex = static_cast<int>(kObstacleTextures.size()) - 1;
        
        return kObstacleTextures[textureIndex];
    }
    
    void Obstacle::Draw ( float overhang )
    {
        float t = Time::Sequencer::Default().Time();
        auto tex = kObstacleTextures[_textureIndex];
        
        static std::vector<vec2> kUVS =
        {
            {  0.0f, 0.0f },
            {  0.0f, 1.0f },
            {  1.0f, 0.0f },
            {  1.0f, 1.0f }
        };
        
        std::vector<vec2> points =
        {
            { -0.5f, -0.5f },
            { -0.5f,  0.5f },
            {  0.5f, -0.5f },
            {  0.5f,  0.5f }
        };
        
        vec2 aspect = { 1.0f, tex->getAspectRatio() };
        
        vec2 position = PositionAt(t);
        //position *= scale;
        //if ( flip ) position.y = simSize.y - position.y;
        
        float radius = RadiusAt(t) * overhang;
        float rotation = RotationAt(t);
        
        if ( radius < 0.01f ) return;
        
        for ( auto& pt : points )
        {
            pt /= aspect;
            pt = glm::rotate ( pt, rotation );
            pt *= radius * overhang;
            pt += position;
        };
        
        gl::VertBatch batch ( GL_TRIANGLE_STRIP );
        gl::ScopedTextureBind tex0 ( tex );
        gl::ScopedGlslProg shader { gl::getStockShader( gl::ShaderDef().texture().color() ) };
        
        for ( int i = 0; i < 4; i++ )
        {
            batch.texCoord0 ( kUVS[i] );
            batch.vertex ( points[i] );
        }
                          
        batch.draw();
    }
    
    void Obstacle::InternalInspect ( )
    {
        Element::InternalInspect ( );
        Time::Inspect( _rotation, "Rotation", IsAngle );
        ui::Combo( "Texture", &_textureIndex, kObstacleTextureNames );
    }
    
    void Obstacle::Serialize ( JsonTree& tree )
    {
        Element::Serialize( tree );
        Time::Serialize( tree, _rotation, "Rotation" );
        tree.pushBack ( JsonTree ( "TextureIndex", _textureIndex ) );
    }
    
    void Obstacle::Marshal ( const ci::JsonTree& tree )
    {
        Element::Marshal( tree );
        
        _rotation = FloatProperty ( tree["Rotation"] );
        _textureIndex = tree["TextureIndex"].getValue<int>();
    }
}
