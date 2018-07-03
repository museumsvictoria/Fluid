//
//  OSCChannel.cxx
//  Fluid
//
//  Created by Andrew Wright on 3/4/18.
//

#include <Time/OSCChannel.h>
#include <asio/io_service.hpp>

using namespace ci;

namespace Time
{
    OSCChannel::OSCChannel ( const std::string& host, int port, Mode mode )
    : _ioService( new asio::io_service )
    , _work( new asio::io_service::work( *_ioService ) )
    , _isConnected ( false )
    , Endpoint ( host )
    , Port ( port )
    , Direction( mode )
    {
        if ( mode == Mode::Outgoing )
        {
            InitSender( host, port );
        }else
        {
            InitReceiver( port );
        }
    }
    
    void OSCChannel::InitSender ( const std::string &host, int port )
    {
        try
        {
            _sender = std::make_unique<Sender>( port + 2, host, port, protocol::v4(), *_ioService );
            _sender->bind();
            _isConnected = true;
            _thread = std::thread( std::bind( []( std::shared_ptr<asio::io_service> &service )
            {
                service->run();
            }, _ioService ));
            
        } catch ( const osc::Exception &ex )
        {
            std::cout << "Error opening OSC Channel!\n " << ex.what() << std::endl;;
            _sender = nullptr;
            _isConnected = false;
        }
    }
    
    void OSCChannel::InitReceiver ( int port )
    {
        try
        {
            _receiver = std::make_unique<Receiver>( port, protocol::v4(), *_ioService );
            _receiver->bind();
            _receiver->listen( [this] ( asio::error_code error, protocol::endpoint endpoint ) -> bool
            {
                if( error )
                {
                    std::cout << "Error Listening: " << error.message() << " val: " << error.value() << " endpoint: " << endpoint << std::endl;
                    return false;
                }else
                {
                    _isConnected = true;
                    return true;
                }
            } );
            
            _thread = std::thread( std::bind( []( std::shared_ptr<asio::io_service> &service )
            {
                service->run();
            }, _ioService ));
            
        }catch ( const std::exception& ex )
        {
            std::cout << "Error opening OSC Channel!\n " << ex.what() << std::endl;;
            _receiver = nullptr;
            _isConnected = false;
        }
    }
    
    void OSCChannel::Listen ( std::function<void(float)> syncHandler )
    {
        if ( _receiver )
        {
            std::lock_guard<std::mutex> lock ( _transportLock );
            _receiver->setListener( "/sync", [syncHandler] ( const osc::Message& message )
            {
                float t = message.getArgFloat(0);
                app::App::get()->dispatchAsync( [syncHandler, t]
                {
                   syncHandler ( t );
                });
            } );
        }
    }
    
    void OSCChannel::SendEvent ( const std::string& event )
    {
        if ( _sender && _isConnected )
        {
            std::lock_guard<std::mutex> lock ( _transportLock );
            osc::Message message { event };
            _sender->send ( message, [] ( asio::error_code error )
            {
                std::cout << error.message() << " : " << error.category().name() << std::endl;
            });
        }
    }
    
    void OSCChannel::SendEvent ( const std::string& event, float value )
    {
        if ( _sender && _isConnected )
        {
            std::lock_guard<std::mutex> lock ( _transportLock );
            osc::Message message { event };
            message.append ( value );
            _sender->send ( message, [] ( asio::error_code error )
            {
                  std::cout << error.message() << " : " << error.category().name() << std::endl;
            });
        }
    }
    
    OSCChannel::~OSCChannel ( )
    {
        if ( _sender )
        {
            std::lock_guard<std::mutex> lock { _transportLock };
            _sender->close();
            _sender = nullptr;
        }
        
        if ( _receiver )
        {
            std::lock_guard<std::mutex> lock { _transportLock };
            _receiver->close();
            _receiver = nullptr;
        }
        
        _work.reset();
        _ioService->stop();
        if ( _thread.joinable() ) _thread.join();
    }
    
}
