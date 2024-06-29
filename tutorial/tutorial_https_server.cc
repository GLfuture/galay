#include "../galay/galay.h"
#include <condition_variable>
#include <spdlog/spdlog.h>
#include <mutex>
#include <signal.h>

galay::GY_TcpCoroutine<galay::CoroutineStatus> test(galay::GY_HttpController::wptr ctrl)
{
    auto request = ctrl.lock()->GetRequest();
    auto response = std::make_shared<galay::protocol::http::Http1_1_Response>();
    response->SetStatus(200) ;
    response->SetVersion("1.1");
    response->SetHeadPair({"Content-Type", "text/html"});
    std::string body = "<html><head><meta charset=\"utf-8\"><title>title</title></head><body>hello world!</body></html>";
    response->SetBody(std::move(body));
    ctrl.lock()->PushResponse(response->EncodePdu());
    if(request->GetHeadValue("connection").compare("close") == 0){
        ctrl.lock()->Close();
    }
    co_return galay::CoroutineStatus::kCoroutineFinished;
}

galay::GY_TcpServer::ptr server;

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