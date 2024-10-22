#include "../galay/galay.h"
#include <iostream>
#include <spdlog/spdlog.h>

int main()
{
    galay::server::TcpServer server;
    server.ReSetNetworkSchedulerNum(8);
    server.ReSetCoroutineSchedulerNum(8);
    galay::CallbackStore store([](galay::TcpOperation op)->galay::coroutine::Coroutine {
        auto connection = op.GetConnection();
        int length = co_await connection->WaitForRecv();
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
            op.SetContext(request);
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
            length = co_await connection->WaitForSend();
        }
        co_return;
    });
    server.Start(&store, 8060, 128);
    getchar();
    server.Stop();
    return 0;
}