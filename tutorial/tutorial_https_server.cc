#include "../galay/galay.h"
#include <condition_variable>
#include <spdlog/spdlog.h>
#include <mutex>
#include <signal.h>

galay::common::GY_NetCoroutine<galay::common::CoroutineStatus> test(galay::kernel::GY_HttpController::wptr ctrl)
{
    auto request = ctrl.lock()->GetRequest();
    auto response = std::make_shared<galay::protocol::http::HttpResponse>();
    response->Header()->Code() = 200;
    response->Header()->Version() = "1.1";
    response->Header()->Headers()["Content-Type"] = "text/html";
    std::string body = "<html><head><meta charset=\"utf-8\"><title>title</title></head><body>hello world!</body></html>";
    response->Body() = std::move(body);
    ctrl.lock()->PushResponse(response->EncodePdu());
    co_return galay::common::CoroutineStatus::kCoroutineFinished;
}

galay::kernel::GY_TcpServer::ptr server;

void signal_handler(int signo)
{
    if (signo == SIGINT)
    {
        server->Stop();
    }
}

int main()
{
    signal(SIGINT,signal_handler);
    spdlog::set_level(spdlog::level::debug);
    auto router = galay::GY_RouterFactory::CreateHttpRouter();
    router->Get("/",test);
    auto builder = galay::GY_ServerBuilderFactory::CreateHttpsServerBuilder("./server.key","./server.crt",8082,router);
    server = galay::GY_ServerFactory::CreateHttpsServer(builder);
    server->Start();
    return 0;
}