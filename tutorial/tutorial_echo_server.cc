#include "galay/galay.h"
#include <iostream>
#include <spdlog/spdlog.h>

int main()
{
    spdlog::set_level(spdlog::level::debug);
    // galay::server::TcpServer server;
    // galay::TcpCallbackStore store([](galay::TcpOperation op)->galay::coroutine::Coroutine {
    //     auto connection = op.GetConnection();
    //     int length = co_await connection->WaitForRecv();
    //     if( length == 0 ) {
    //         auto data = connection->FetchRecvData();
    //         data.Clear();
    //         bool b = co_await connection->CloseConnection();
    //         co_return;
    //     }
    //     auto data = connection->FetchRecvData();
    //     galay::protocol::http::HttpRequest::ptr request = std::make_shared<galay::protocol::http::HttpRequest>();
    //     int elength = request->DecodePdu(data.Data());
    //     if(request->HasError()) {
    //         if( elength >= 0 ) {
    //             data.Erase(elength);
    //         } 
    //         if( request->GetErrorCode() == galay::error::HttpErrorCode::kHttpError_HeaderInComplete || request->GetErrorCode() == galay::error::HttpErrorCode::kHttpError_BodyInComplete)
    //         {
    //             op.GetContext() = request;
    //             op.ReExecute(op);
    //         } else {
    //             data.Clear();
    //             bool b = co_await connection->CloseConnection();
    //             co_return;
    //         }
    //     }
    //     else{
    //         data.Clear();
    //         galay::protocol::http::HttpResponse response;
    //         response.Header()->Version() = "1.1";
    //         response.Header()->Code() = 200;
    //         response.Header()->HeaderPairs().AddHeaderPair("Content-Type", "text/html");
    //         response.Body() = "Hello World";
    //         std::string respStr = response.EncodePdu();
    //         connection->PrepareSendData(respStr);
    //         length = co_await connection->WaitForSend();
    //         bool b = co_await connection->CloseConnection();
    //     }
    //     co_return;
    // });
    //server.Start(&store, 8060);

    galay::server::HttpServer server;
    server.Get("/", [](galay::HttpOperation op) ->galay::coroutine::Coroutine {
        auto resp = op.GetResponse();
        resp->Header()->Version() = "1.1";
        resp->Header()->Code() = galay::protocol::http::OK_200;
        resp->Header()->HeaderPairs().AddHeaderPair("Content-Type", "text/html");
        resp->Header()->HeaderPairs().AddHeaderPair("Server", "Galay");
        resp->Body() = "<h1>Hello World</h1>";
        std::string response = resp->EncodePdu();
        int res = co_await op.ReturnResponse(response);
        bool ret = co_await op.CloseConnection();
        spdlog::info("Response: {}", res);
        co_return;
    });
    server.Start(8060);
    getchar();
    server.Stop();
    return 0;
}