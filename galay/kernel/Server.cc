#include "Server.hpp"
#include "Scheduler.h"
#include "Coroutine.hpp"

namespace galay::details
{

Coroutine<void> CreateConnection(RoutineCtx ctx, galay::AsyncTcpSocket *socket, EventScheduler *scheduler)
{
    THost addr{};
    while(true)
    {
        GHandle handle = co_await socket->Accept(&addr);
        if( handle.fd == -1 ){
            if(const uint32_t error = socket->GetErrorCode(); error != error::Error_NoError ) {
                LogError("[{}]", error::GetErrorString(error));
            }
            co_return;
        }
        auto new_socket = std::make_unique<galay::AsyncTcpSocket>();
        if( !new_socket->Socket(handle) ) {
            co_return;
        }
        LogTrace("[Handle:{}, Acceot Success]", new_socket->GetHandle().fd);
        scheduler->GetEngine()->ResetMaxEventSize(handle.fd);
        CallbackStore<galay::AsyncTcpSocket>::CreateConnAndExecCallback(scheduler, std::move(new_socket));
    }
    co_return;
}

Coroutine<void> CreateConnection(RoutineCtx ctx, AsyncTcpSslSocket *socket, EventScheduler *scheduler)
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
        auto new_socket = std::unique_ptr<AsyncTcpSslSocket>();
        if( bool res = new_socket->Socket(handle); !res ) {
            co_return;
        }
        LogTrace("[Handle:{}, Accept Success]", new_socket->GetHandle().fd);
        if(bool success = co_await new_socket->SSLAccept(); !success ){
            LogError("[{}]", error::GetErrorString(new_socket->GetErrorCode()));
            close(handle.fd);
            co_return;
        }
        LogTrace("[Handle:{}, SSL_Acceot Success]", new_socket->GetHandle().fd);
        scheduler->GetEngine()->ResetMaxEventSize(handle.fd);
        CallbackStore<AsyncTcpSslSocket>::CreateConnAndExecCallback(scheduler, std::move(new_socket));
    }
    co_return;
}

}

namespace galay {

TcpServerConfig::ptr TcpServerConfig::Create()
{
    return std::make_shared<TcpServerConfig>();
}

}