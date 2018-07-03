//
//  PingPongBuffer.cxx
//  Fluid
//
//  Created by Andrew Wright on 29/6/17.
//
//

#include "PingPongBuffer.h"

using namespace ci;

namespace Fluid
{
    PingPongBufferRef PingPongBuffer::Create ( int width, int height, const gl::Fbo::Format& format )
    {
        return PingPongBufferRef ( new PingPongBuffer ( width, height, format ) );
    }
    
    PingPongBuffer::PingPongBuffer ( int width, int height, const gl::Fbo::Format& format )
    {
        _buffers[0] = gl::Fbo::create ( width, height, format );
        _buffers[1] = gl::Fbo::create ( width, height, format );
        
        Clear();
    }
    
    void PingPongBuffer::Draw ( const Rectf& bounds )
    {
        gl::draw ( SourceTexture(), bounds );
    }
    
    void PingPongBuffer::Clear ( const ColorAf& color )
    {
        {
            gl::ScopedFramebuffer buffer { _buffers[0] };
            gl::clear( color );
        }
        
        {
            gl::ScopedFramebuffer buffer { _buffers[1] };
            gl::clear( color );
        }
        
        _ping = 0;
    }
    
    void PingPongBuffer::ClearCurrent ( const ColorAf& color )
    {
        {
            gl::ScopedFramebuffer buffer { DestinationBuffer() };
            gl::clear( color );
        }
    }
    
    void PingPongBuffer::Swap ( )
    {
        _ping = 1 - _ping;
    }
    
    const gl::FboRef& PingPongBuffer::SourceBuffer ( ) const
    {
        return _buffers[_ping];
    }
    
    const gl::FboRef& PingPongBuffer::DestinationBuffer ( ) const
    {
        return _buffers[1 - _ping];
    }
    
    const gl::TextureRef PingPongBuffer::SourceTexture ( ) const
    {
        return SourceBuffer()->getColorTexture();
    }
    
    const gl::TextureRef PingPongBuffer::DestinationTexture ( ) const
    {
        return DestinationBuffer()->getColorTexture();
    }
}
