#include "../galay/galay.h"
#include <spdlog/spdlog.h>
#include <iostream>
#include <signal.h>

//必须放在外面，不然在多线程中唤醒协程，协程中析构client时会析构future导致死锁
galay::GY_HttpAsyncClient client;

galay::GY_TcpCoroutine<galay::CoroutineStatus> test_http_proxy(galay::GY_HttpController::wptr ctrl)
{
    auto request = std::dynamic_pointer_cast<galay::protocol::http::Http1_1_Request>(ctrl.lock()->GetRequest());
    auto response = std::make_shared<galay::protocol::http::Http1_1_Response>();
    auto response1 = co_await client.Get("http://183.2.172.42:80");
    client.Close();
    response->SetBody(response1->GetBody());
    response->SetStatus(response1->GetStatus());
    response->SetVersion(response1->GetVersion());
    response->SetHeaders(response1->GetHeaders());
    ctrl.lock()->PushResponse(response);
    ctrl.lock()->Done();
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
    router->Get("/echo",test_http_proxy);
    galay::GY_HttpServerBuilder::ptr builder = galay::GY_ServerBuilderFactory::CreateHttpServerBuilder(8080,router);
    server = galay::GY_ServerFactory::CreateHttpServer(builder);
    server->Start();
    return 0;
}