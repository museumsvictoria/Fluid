//
//  ImageSequence.cxx
//  Fluid
//
//  Created by Andrew Wright on 24/3/18.
//

#include "ImageSequence.h"

using namespace ci;

const ci::gl::TextureRef& ImageSequence::FrameData::Texture ( )
{
    if ( _texture ) return _texture;
    
    auto fmt = gl::Texture::Format().mipmap().minFilter(GL_LINEAR_MIPMAP_LINEAR).magFilter(GL_LINEAR);
    _texture = gl::Texture::create ( loadImage ( loadFile ( Path ) ), fmt );
    
    return _texture;
}

ImageSequence::ImageSequence ( const fs::path& folder, bool autoUpdate )
{
    fs::directory_iterator it { folder }, end;
    
    while ( it != end )
    {
        if ( it->path().extension().string() == ".png" )
        {
            FrameData f;
            f.Path = it->path();
            _frames.push_back ( f );
        }
        it++;
    }
    
    if ( !_frames.empty() )
    {
        _size = _frames[0].Texture()->getSize();
        if ( autoUpdate )
        {
            _updateConnection = app::App::get()->getSignalUpdate().connect ( [=]
            {
                if ( Playing )
                {
                    _currentFrame += Rate;
                    if ( Loops )
                    {
                        if ( _currentFrame >= _frames.size() - 1 ) _currentFrame = 0;
                        if ( _currentFrame < 0 ) _currentFrame = static_cast<float>(_frames.size() - 1);
                    }else
                    {
                        if ( _currentFrame >= _frames.size() - 1 ) _currentFrame = (int)_frames.size() - 1;
                        if ( _currentFrame < 0 ) _currentFrame = 0;
                    }
                }
            } );
        }
    }
}

int ImageSequence::CurrentFrame ( ) const
{
    auto f = std::round ( _currentFrame );
    if ( f < 0 ) f = 0;
    if ( f > TotalFrames() - 1 ) f = TotalFrames() - 1;
    
    return f;
    
};

ImageSequence::~ImageSequence ( )
{
    _updateConnection.disconnect();
}
