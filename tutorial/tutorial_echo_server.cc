#include "galay/galay.h"
#include <iostream>
#include <spdlog/spdlog.h>
#define BUFFER_SIZE 1024
int main()
{
    auto& logger = galay::log::Logger::GetInstance()->GetLogger();
    logger->set_level(spdlog::level::trace);
    logger->flush_on(spdlog::level::trace);
    int level = logger->level();
    std::cout << "level: " << level << std::endl;
    galay::server::TcpServerConfig::ptr config = std::make_shared<galay::server::TcpServerConfig>();
    galay::server::TcpServer server(config);
    galay::TcpCallbackStore store([](galay::TcpOperation op)->galay::coroutine::Coroutine {
        auto connection = op.GetConnection();
        galay::async::AsyncNetIo* socket = connection->GetSocket();
        galay::IOVec iov {};
        galay::VecMalloc(&iov, BUFFER_SIZE);
        galay::protocol::http::HttpRequest request;
        galay::protocol::http::HttpResponse response;
        iov.m_length = BUFFER_SIZE;
        while(true)
        {
            int length = co_await galay::AsyncRecv(socket, &iov);
            std::cout << "recv length: " << length << std::endl;
            if( length <= 0 ) {
                if( length == galay::event::CommonFailedType::eCommonDisConnect || length == galay::event::CommonFailedType::eCommonOtherFailed ) {
                    free(iov.m_buffer);
                    co_await galay::AsyncClose(socket);
                    co_return;
                }
            } 
            else {
                std::string_view data(iov.m_buffer, iov.m_offset + length);
                auto result = request.DecodePdu(data);
                spdlog::info("DecodePdu result: {}, {}", result.first, request.GetErrorCode());
                if(!result.first) {
                    switch (request.GetErrorCode())
                    {
                    case galay::error::HttpErrorCode::kHttpError_HeaderInComplete:
                    case galay::error::HttpErrorCode::kHttpError_BodyInComplete:
                    {
                        iov.m_offset += length;
                        if( iov.m_offset >= iov.m_size ) {
                            //buffer不够，扩容
                            galay::VecRealloc(&iov, iov.m_size + BUFFER_SIZE);
                        }
                        break;
                    }
                    case galay::error::HttpErrorCode::kHttpError_BadRequest:
                    {
                        
                    }
                    default:
                        break;
                    }
                } else {
                    response.Header()->Code() = galay::protocol::http::HttpStatusCode::OK_200;
                    response.Header()->Version() = galay::protocol::http::HttpVersion::Http_Version_1_1;
                    response.Header()->HeaderPairs().AddHeaderPair("Content-Type", "text/html");
                    response.Body() = "<html> <h1> Hello World </h1> </html>";
                    std::string res_str = response.EncodePdu();
                    galay::IOVec wiov{
                        .m_buffer = res_str.data(),
                        .m_size = res_str.length(),
                        .m_offset = 0,
                        .m_length = res_str.length()
                    };
                    while(true) {
                        int send_len = co_await galay::AsyncSend(socket, &wiov);
                        std::cout << "send_len: " << send_len << std::endl;
                        if(send_len > 0) {
                            wiov.m_offset += send_len;
                            if(wiov.m_offset == wiov.m_size) break;
                        } else {
                            free(iov.m_buffer);
                            co_await galay::AsyncClose(socket);
                            co_return;
                        }
                    }
                }
                request.Reset();
                response.Reset();
            }
        }
    end:
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
    std::cout << "level: " << level << std::endl;
    server.Stop();
    return 0;
}