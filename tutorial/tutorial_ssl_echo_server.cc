#include "galay/galay.h"
#include <spdlog/spdlog.h>
#include <iostream>

int main(int argc, char** argv)
{
    if(argc < 3) {
        std::cout << "Usage: " << argv[0] << " <certificate.crt> <private.key>" << std::endl;
        return -1;
    }
    spdlog::set_level(spdlog::level::debug);
    galay::server::TcpSslServerConfig::ptr config = std::make_shared<galay::server::TcpSslServerConfig>();
    config->m_cert_file = argv[1];
    config->m_key_file = argv[2];
    galay::server::TcpSslServer server(config);
    galay::TcpSslCallbackStore store([](galay::TcpSslOperation op)->galay::coroutine::Coroutine
    {
        auto connection = op.GetConnection();
        galay::async::AsyncSslNetIo* socket = connection->GetSocket();
        struct galay::async::NetIOVec iov {
            .m_buf = new char[1024],
            .m_len = 1024
        };
        int length = co_await galay::async::AsyncSSLRecv(socket, &iov);
        galay::protocol::http::HttpRequest::ptr request = std::make_shared<galay::protocol::http::HttpRequest>();
        std::string_view data(iov.m_buf, length);
        int elength = request->DecodePdu(data);
        if( length == 0 ) {
            goto end;
        }
        if(request->HasError()) {
            if( request->GetErrorCode() == galay::error::HttpErrorCode::kHttpError_HeaderInComplete || request->GetErrorCode() == galay::error::HttpErrorCode::kHttpError_BodyInComplete)
            {
                op.GetContext() = request;
                op.ReExecute(op);
                co_return;
            } 
            goto end;
        }
        else{
            galay::protocol::http::HttpResponse response;
            response.Header()->Version() = galay::protocol::http::HttpVersion::Http_Version_1_1;
            response.Header()->Code() = galay::protocol::http::HttpStatusCode::OK_200;
            response.Header()->HeaderPairs().AddHeaderPair("Content-Type", "text/html");
            response.Body() = "Hello World";
            std::string respStr = response.EncodePdu();
            galay::async::NetIOVec iov2{
                .m_buf = respStr.data(),
                .m_len = respStr.size()
            };
            length = co_await galay::async::AsyncSSLSend(socket, &iov2);
            goto end;
        }
    end:
        bool b = co_await galay::async::AsyncSSLClose(socket);
        delete[] iov.m_buf;
        co_return;
    });
    server.Start(&store, 2333);
    getchar();
    server.Stop();
    return 0;
}