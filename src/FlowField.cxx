//
//  FlowField.cxx
//  Fluid
//
//  Created by Andrew Wright on 25/3/18.
//

#include "FlowField.h"
#include <Time/Sequencer.h>
#include "CinderImgui.h"

using namespace ci;

static gl::BatchRef kLineBatch = nullptr;

FlowField::FlowField ( Fluid::Sim * fluid )
: _fluid( fluid )
{
    Alpha = 0.0f;
    
    if ( !kLineBatch )
    {
        {
            int xRes = 2;
            int yRes = 2;
            
            std::vector<vec2> points;
            
            for ( int y = 0; y < app::getWindowHeight() * fluid->Scale(); y += yRes )
            {
                for ( int x = 0; x < app::getWindowWidth() * fluid->Scale(); x += xRes )
                {
                    vec2 a { x, y };
                    vec2 b { x + xRes, y };
                    
                    points.push_back( a );
                    points.push_back( b );
                }
            }
            
            auto layout = gl::VboMesh::Layout().attrib( geom::POSITION, 2 );
            auto mesh = gl::VboMesh::create( (int)points.size(), GL_LINES, { layout } );
            mesh->bufferAttrib( geom::POSITION, points );
            kLineBatch = gl::Batch::create ( mesh, gl::GlslProg::create( app::loadAsset( "Shaders/Rendering/LineSegment.vs.glsl" ), app::loadAsset( "Shaders/Rendering/LineSegment.fs.glsl" ) ) );
            kLineBatch->getGlslProg()->uniform ( "uVelocity", 0 );
            kLineBatch->getGlslProg()->uniform ( "uDensity", 1 );
        }
    }
}

void FlowField::Inspect ( )
{
    if ( ui::CollapsingHeader ( "Flow Field" ) )
    {
        ui::ScopedId id { "FlowField" };
        Time::Inspect ( Alpha, "Alpha", Time::BetweenZeroAndOne );
        Time::Inspect ( ColorWeight, "Colour Weight", Time::BetweenZeroAndOne );
        Time::Inspect ( Weight, "Distortion Weight", Time::BetweenZeroAndOne );
    }
}

void FlowField::Draw ( )
{
    auto t = Time::Sequencer::Default().Time();
    float a = Alpha.ValueAtTime(t);
    
    if ( a > 0.0f )
    {
        float w = ColorWeight.ValueAtTime(t);
        kLineBatch->getGlslProg()->uniform( "uAlpha", a );
        kLineBatch->getGlslProg()->uniform( "uColorWeight", w );
        kLineBatch->getGlslProg()->uniform( "uWeight", Weight.ValueAtTime(t) );
        
        gl::ScopedTextureBind tex0 ( _fluid->GetVelocity(), 0 );
        gl::ScopedTextureBind tex1 ( _fluid->GetDensity(), 1 );
        gl::ScopedMatrices mat;
        gl::ScopedColor color { ColorAf ( 1, 1, 1, a ) };
        
        gl::scale ( vec2 ( 1.0 / _fluid->Scale() ) );
        gl::ScopedBlendAlpha blend;
        kLineBatch->getGlslProg()->uniform( "uSize", vec2 ( app::getWindowSize() ) * _fluid->Scale() );
        kLineBatch->draw();
    }
}
