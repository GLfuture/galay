#include "galay/galay.h"
#include <iostream>
#include <spdlog/spdlog.h>


int main()
{
    spdlog::set_level(spdlog::level::debug);
    galay::server::TcpServer server;
    galay::TcpCallbackStore store([](galay::TcpOperation op)->galay::coroutine::Coroutine {
        auto connection = op.GetConnection();
        galay::async::AsyncTcpSocket* socket = connection->GetSocket();
        struct galay::async::IOVec iov {
            .m_buf = new char[1024],
            .m_len = 1024
        };
        int length = co_await galay::async::AsyncRecv(socket, &iov);
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
            galay::async::IOVec iov2{
                .m_buf = respStr.data(),
                .m_len = respStr.size()
            };
            length = co_await galay::async::AsyncSend(socket, &iov2);
            goto end;
        }
    end:
        bool b = co_await galay::async::AsyncClose(socket);
        delete[] iov.m_buf;
        co_return;
    });
    server.Start(&store, 8060);
    // galay::server::HttpServer server;
    // server.Get("/", [](galay::HttpOperation op) ->galay::coroutine::Coroutine {
    //     auto resp = op.GetResponse();
    //     resp->Header()->Version() = galay::protocol::http::HttpVersion::Http_Version_1_1;
    //     resp->Header()->Code() = galay::protocol::http::HttpStatusCode::OK_200;
    //     resp->Header()->HeaderPairs().AddHeaderPair("Content-Type", "text/html");
    //     resp->Header()->HeaderPairs().AddHeaderPair("Server", "Galay");
    //     resp->Body() = "<h1>Hello World</h1>";
    //     std::string response = resp->EncodePdu();
    //     int res = co_await op.ReturnResponse(response);
    //     bool ret = co_await op.CloseConnection();
    //     spdlog::info("Response: {}", res);
    //     co_return;
    // });
    // server.Start(8060);
    getchar();
    server.Stop();
    return 0;
}