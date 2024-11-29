#include "galay/galay.h"
#include <iostream>
#include <spdlog/spdlog.h>
#define BUFFER_SIZE 1024

int main()
{
    spdlog::set_level(spdlog::level::debug);
    galay::server::TcpServerConfig::ptr config = std::make_shared<galay::server::TcpServerConfig>();
    galay::server::TcpServer server(config);
    galay::TcpCallbackStore store([](galay::TcpOperation op)->galay::coroutine::Coroutine {
        auto connection = op.GetConnection();
        galay::async::AsyncNetIo* socket = connection->GetSocket();
        galay::IOVec iov {};
        galay::VecMalloc(&iov, BUFFER_SIZE);
        galay::protocol::http::HttpRequest request;
        while(true)
        {
            int length = co_await galay::AsyncRecv(socket, &iov);
            if( length <= 0 ) {
                if( length == galay::event::CommonFailedType::eCommonDisConnect || length == galay::event::CommonFailedType::eCommonOtherFailed ) {
                    free(iov.m_buffer);
                    bool b = co_await galay::AsyncClose(socket);
                    co_return;
                }
            } 
            else {
                std::string_view data(iov.m_buffer, iov.m_offset + length);
                auto result = request.DecodePdu(data);
                if(!result.first) {
                    if( result.second == galay::error::HttpErrorCode::kHttpError_HeaderInComplete || result.second == galay::error::HttpErrorCode::kHttpError_BodyInComplete ) {
                        iov.m_offset += length;
                        iov.m_length -= length;
                        //如果缓冲区不够 就重新分配 
                        if(iov.m_offset == 1024) {
                            iov.m_buffer = (char*)realloc(iov.m_buffer, iov.m_offset + 1024);
                        } 
                    }
                }
            }
        }
        co_return;
    });
    server.Start(&store, 8060);
    // galay::server::HttpServerConfig::ptr config = std::make_shared<galay::server::HttpServerConfig>();
    // galay::server::HttpServer server(config);
    // server.Get("/", [](galay::HttpOperation op) ->galay::coroutine::Coroutine {
    //     auto resp = op.GetResponse();
    //     resp->Header()->Version() = galay::protocol::http::HttpVersion::Http_Version_1_1;
    //     resp->Header()->Code() = galay::protocol::http::HttpStatusCode::OK_200;
    //     resp->Header()->HeaderPairs().AddHeaderPair("Content-Type", "text/html");
    //     resp->Header()->HeaderPairs().AddHeaderPair("Server", "Galay");
    //     resp->Body() = "<h1>Hello World</h1>";
    //     std::string response = resp->EncodePdu();
    //     galay::async::IOVec vec;
    //     vec.m_buffer = (char*)response.data();
    //     vec.m_length = response.size();
    //     co_await galay::async::AsyncSend(op.GetConnection()->GetSocket(), &vec);
    //     co_return;
    // });
    // server.Start(8060);
    getchar();
    server.Stop();
    return 0;
}