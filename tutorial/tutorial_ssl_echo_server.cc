#include "../galay/galay.h"
#include <spdlog/spdlog.h>
#include <iostream>

int main(int argc, char** argv)
{
    if(argc < 3) {
        std::cout << "Usage: " << argv[0] << " <certificate.crt> <private.key>" << std::endl;
        return -1;
    }
    spdlog::set_level(spdlog::level::debug);
    galay::server::TcpSslServer server(argv[1], argv[2]);
    server.ReSetNetworkSchedulerNum(1);
    server.ReSetCoroutineSchedulerNum(1);
    galay::TcpSslCallbackStore store([](galay::TcpSslOperation op)->galay::coroutine::Coroutine
    {
        auto connection = op.GetConnection();
        int length = co_await connection->WaitForSslRecv();
        if( length == 0 ) {
            auto data = connection->FetchRecvData();
            data.Clear();
            bool b = co_await connection->CloseConnection();
            co_return;
        }
        auto data = connection->FetchRecvData();
        galay::protocol::http::HttpRequest::ptr request = std::make_shared<galay::protocol::http::HttpRequest>();
        int elength = request->DecodePdu(data.Data());
        if(request->ParseIncomplete()) {
            if( elength >= 0 ) {
                data.Erase(elength);
            } 
            op.GetContext() = request;
            op.ReExecute(op);
        }
        else{
            data.Clear();
            galay::protocol::http::HttpResponse response;
            response.Header()->Version() = "1.1";
            response.Header()->Code() = 200;
            response.Header()->HeaderPairs().AddHeaderPair("Content-Type", "text/html");
            response.Body() = "Hello World";
            std::string respStr = response.EncodePdu();
            connection->PrepareSendData(respStr);
            length = co_await connection->WaitForSslSend();
            bool b = co_await connection->CloseConnection();
        }
        co_return;
    });
    server.Start(&store, 2333, 128);
    getchar();
    server.Stop();
    return 0;
}