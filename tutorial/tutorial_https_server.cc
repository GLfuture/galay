#include "../galay/galay.h"
#include <condition_variable>
#include <spdlog/spdlog.h>
#include <mutex>
#include <signal.h>

galay::GY_TcpCoroutine<galay::CoroutineStatus> test(galay::GY_Controller::wptr ctrl)
{
    auto request = std::dynamic_pointer_cast<galay::protocol::http::Http1_1_Request>(ctrl.lock()->GetRequest());
    auto response = std::make_shared<galay::protocol::http::Http1_1_Response>();
    response->SetStatus(200) ;
    response->SetVersion("1.1");
    response->SetHeadPair({"Content-Type", "text/html"});
    std::string body = "<html><head><meta charset=\"utf-8\"><title>title</title></head><body>hello world!</body></html>";
    response->SetBody(std::move(body));
    ctrl.lock()->PushResponse(response);
    if(request->GetHeadValue("connection").compare("close") == 0){
        ctrl.lock()->Close();
    }
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
    builder->InitSSLServer(true);
    builder->GetSSLConfig()->SetCertPath("./server.crt");
    builder->GetSSLConfig()->SetKeyPath("./server.key");
    builder->SetSchedulerType(galay::GY_TcpServerBuilderBase::SchedulerType::SELECT_SCHEDULER);
    builder->SetUserFunction({8082,test});
    builder->SetThreadNum(1);
    server.Start(builder);
    return 0;
}