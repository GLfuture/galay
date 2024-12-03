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
    galay::server::TcpServerConfig::ptr config = std::make_shared<galay::server::TcpServerConfig>();
    galay::server::TcpServer server(config);
    galay::TcpCallbackStore store([](galay::TcpConnectionManager op)->galay::coroutine::Coroutine {
        auto connection = op.GetConnection();
        galay::async::AsyncNetIo* socket = connection->GetSocket();
        galay::IOVecHolder rholder(BUFFER_SIZE), wholder;
        galay::protocol::http::HttpRequest request;
        galay::protocol::http::HttpResponse response;
        while(true)
        {
            int length = co_await galay::AsyncRecv(socket, &rholder, BUFFER_SIZE);
            std::cout << "recv length: " << length << std::endl;
            if( length <= 0 ) {
                if( length == galay::event::CommonFailedType::eCommonDisConnect || length == galay::event::CommonFailedType::eCommonOtherFailed ) {
                    co_await galay::AsyncClose(socket);
                    co_return;
                }
            } 
            else {
                std::string_view data(rholder->m_buffer, rholder->m_offset);
                auto result = request.DecodePdu(data);
                spdlog::info("DecodePdu result: {}, {}", result.first, request.GetErrorCode());
                if(!result.first) {
                    switch (request.GetErrorCode())
                    {
                    case galay::error::HttpErrorCode::kHttpError_HeaderInComplete:
                    case galay::error::HttpErrorCode::kHttpError_BodyInComplete:
                    {
                        if( rholder->m_offset >= rholder->m_size ) {
                            rholder.Realloc(rholder->m_size + BUFFER_SIZE);
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
                    wholder.Reset(response.EncodePdu());
                    while(true) {
                        int send_len = co_await galay::AsyncSend(socket, &wholder, wholder->m_size);
                        std::cout << "send_len: " << send_len << std::endl;
                        if(send_len > 0) {
                            if(wholder->m_offset == wholder->m_size) break;
                        } else {
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
    getchar();
    server.Stop();
    return 0;
}