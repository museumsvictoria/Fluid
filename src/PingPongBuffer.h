//
//  PingPongBuffer.h
//  Fluid
//
//  Created by Andrew Wright on 29/6/17.
//
//

#ifndef Fluid_PingPongBuffer_h
#define Fluid_PingPongBuffer_h

namespace Fluid
{
    using PingPongBufferRef = std::unique_ptr<class PingPongBuffer>;
    class PingPongBuffer
    {
    public:
        
        friend struct ScopedPingPong;
        
        static PingPongBufferRef    Create ( int width, int height, const ci::gl::Fbo::Format& format );
        
        void                        Draw                ( const ci::Rectf& bounds );
        void                        Clear               ( const ci::ColorAf& color = ci::ColorAf::black() );
        void                        ClearCurrent        ( const ci::ColorAf& color = ci::ColorAf::black() );
        void                        Swap                ( );
        
        const ci::gl::FboRef&       SourceBuffer        ( ) const;
        const ci::gl::FboRef&       DestinationBuffer   ( ) const;
        
        const ci::gl::TextureRef    SourceTexture       ( ) const;
        const ci::gl::TextureRef    DestinationTexture  ( ) const;
        
    protected:
        
        PingPongBuffer              ( int width, int height, const ci::gl::Fbo::Format& format );
        
        ci::gl::FboRef              _buffers[2];
        int                         _ping{0};
    };
}

#endif /* Fluid_PingPongBuffer_h */
