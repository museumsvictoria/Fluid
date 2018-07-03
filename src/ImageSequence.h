//
//  ImageSequence.h
//  Fluid
//
//  Created by Andrew Wright on 24/3/18.
//

#ifndef Fluid_ImageSequence_h
#define Fluid_ImageSequence_h

#include "cinder/Signals.h"

class ImageSequence
{
public:
    
    struct FrameData
    {
        ci::fs::path                Path;
        const ci::gl::TextureRef&   Texture ( );
        
    protected:
        ci::gl::TextureRef          _texture{nullptr};
    };
    
    ImageSequence               ( ) { };
    ImageSequence               ( const ci::fs::path& folder, bool autoUpdate = true );
    ~ImageSequence              ( );
    
    const ci::ivec2&            Size            ( ) const { return _size; };
    FrameData&                  Frame           ( ) { return _frames[CurrentFrame()]; };
    const FrameData&            Frame           ( ) const { return _frames[CurrentFrame()]; };
    const ci::gl::TextureRef&   CurrentTexture  ( ) { return Frame().Texture(); }
    
    int                         CurrentFrame    ( ) const;
    inline int                  TotalFrames     ( ) const { return (int)_frames.size(); };
    
    void                        GotoAndPlay     ( int frame ) { _currentFrame = frame; Playing = true; }
    void                        GotoAndStop     ( int frame ) { _currentFrame = frame; Playing = false; }
    
    void                        DrawInRect      ( const ci::Rectf& rect ) { ci::gl::draw ( CurrentTexture(), rect ); };
    
    bool                        Loops{true};
    bool                        Playing{true};
    float                       Rate = 0.5f;
    
protected:
    
    std::vector<FrameData>      _frames;
    float                       _currentFrame{0.0f};
    ci::ivec2                   _size;
    ci::signals::Connection     _updateConnection;
};

#endif /* ImageSequence_h */
