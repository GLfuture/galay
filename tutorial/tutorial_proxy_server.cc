#include "../galay/galay.h"
#include <spdlog/spdlog.h>
#include <iostream>
#include <signal.h>


galay::GY_TcpCoroutine<galay::CoroutineStatus> test_http_proxy(galay::GY_Controller::wptr ctrl)
{
    auto request = std::dynamic_pointer_cast<galay::protocol::http::Http1_1_Request>(ctrl.lock()->GetRequest());
    auto response = std::make_shared<galay::protocol::http::Http1_1_Response>();
    galay::GY_HttpAsyncClient client("http://183.2.172.185:80");
    auto response1 = co_await client.Get(request);
    client.Close();
    response->SetBody(response1->GetBody());
    response->SetStatus(response1->GetStatus());
    response->SetVersion(response1->GetVersion());
    response->SetHeaders(response1->GetHeaders());
    ctrl.lock()->Done();
    co_return galay::CoroutineStatus::GY_COROUTINE_FINISHED;
}

galay::GY_TcpServer server;

void signal_handler(int signo)
{
    if (signo == SIGINT)
    {
        server.Stop();
    }
}

int main()
{
    signal(SIGINT,signal_handler);
    spdlog::set_level(spdlog::level::debug);
    galay::GY_TcpServerBuilder<galay::protocol::http::Http1_1_Request,galay::protocol::http::Http1_1_Response>::ptr 
    builder = std::make_shared<galay::GY_TcpServerBuilder<galay::protocol::http::Http1_1_Request,galay::protocol::http::Http1_1_Response>>();
    builder->SetSchedulerType(galay::GY_TcpServerBuilderBase::SchedulerType::EPOLL_SCHEDULER);
    builder->SetUserFunction({8082,test_http_proxy});
    builder->SetThreadNum(1);
    server.Start(builder);
    return 0;
}