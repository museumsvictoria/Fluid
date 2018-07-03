//
//  OSCChannel.h
//  Fluid
//
//  Created by Andrew Wright on 3/4/18.
//

#ifndef OSCChannel_h
#define OSCChannel_h

#include "cinder/osc/Osc.h"

namespace Time
{
    using Sender        = ci::osc::SenderUdp;
    using Receiver      = ci::osc::ReceiverUdp;
    
    using SenderRef     = std::unique_ptr<Sender>;
    using ReceiverRef   = std::unique_ptr<Receiver>;
    
    using protocol      = asio::ip::udp;
    
    using OSCChannelRef = std::unique_ptr<class OSCChannel>;
    class OSCChannel
    {
    public:
        
        enum class Mode
        {
            Outgoing,
            Incoming
        };
        
        OSCChannel                                  ( const std::string& host, int port, Mode mode = Mode::Outgoing );
        ~OSCChannel                                 ( );
        
        void                                        SendEvent ( const std::string& event );
        void                                        SendEvent ( const std::string& event, float value );
        void                                        Listen    ( std::function<void(float)> syncHandler );
        
        std::string                                 Endpoint;
        int                                         Port{9001};
        Mode                                        Direction;
        
    protected:
        
        void                                        InitSender   ( const std::string& host, int port );
        void                                        InitReceiver ( int port );
        
        std::shared_ptr<asio::io_service>           _ioService;
        std::shared_ptr<asio::io_service::work>     _work;
        std::thread                                 _thread;
        std::mutex                                  _transportLock;
        SenderRef                                   _sender;
        ReceiverRef                                 _receiver;
        bool                                        _isConnected{false};
    };
}

#endif /* OSCChannel_h */
