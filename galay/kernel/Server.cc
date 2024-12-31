#include "Server.hpp"
#include "Scheduler.h"
#include "Coroutine.hpp"

namespace galay::details
{

Coroutine<void> CreateConnection(RoutineCtx::ptr ctx, galay::AsyncTcpSocket *socket, CallbackStore<galay::AsyncTcpSocket> *store, EventEngine *engine)
{
    THost addr{};
    while(true)
    {
        const GHandle handle = co_await socket->Accept(&addr);
        if( handle.fd == -1 ){
            if(const uint32_t error = socket->GetErrorCode(); error != error::Error_NoError ) {
                LogError("[{}]", error::GetErrorString(error));
            }
            co_return;
        }
        auto new_socket = new galay::AsyncTcpSocket(engine);
        if( !new_socket->Socket(handle) ) {
            delete new_socket;
            co_return;
        }
        LogTrace("[Handle:{}, Acceot Success]", new_socket->GetHandle().fd);
        engine->ResetMaxEventSize(handle.fd);
        store->Execute(new_socket);
    }
    co_return;
}

Coroutine<void> CreateConnection(RoutineCtx::ptr ctx, AsyncTcpSslSocket *socket, CallbackStore<AsyncTcpSslSocket> *store, EventEngine *engine)
{
    THost addr{};
    while (true)
    {
        const auto handle = co_await socket->Accept(&addr);
        if( handle.fd == -1 ){
            if(const uint32_t error = socket->GetErrorCode(); error != error::Error_NoError ) {
                LogError("[{}]", error::GetErrorString(error));
            }
            co_return;
        }
        auto new_socket = new AsyncTcpSslSocket(engine);
        if( bool res = new_socket->Socket(handle); !res ) {
            delete new_socket;
            co_return;
        }
        LogTrace("[Handle:{}, Accept Success]", new_socket->GetHandle().fd);
        if(const bool success = co_await new_socket->SSLAccept(); !success ){
            LogError("[{}]", error::GetErrorString(new_socket->GetErrorCode()));
            close(handle.fd);
            delete new_socket;
            co_return;
        }
        LogTrace("[Handle:{}, SSL_Acceot Success]", new_socket->GetHandle().fd);
        engine->ResetMaxEventSize(handle.fd);
        store->Execute(new_socket);
    }
    co_return;
}



}


namespace galay::server
{

TcpServerConfig::ptr TcpServerConfig::Create()
{
    return std::make_shared<TcpServerConfig>();
}


HttpServerConfig::ptr galay::server::HttpServerConfig::Create()
{
    return std::make_shared<HttpServerConfig>();
}


}
