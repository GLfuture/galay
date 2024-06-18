#include "../galay/galay.h"
#include <condition_variable>
#include <spdlog/spdlog.h>
#include <mutex>
#include <signal.h>

#define ILLEGAL_NOT_CLOSE 0
#define ILLEGAL_DELAY_CLOSE 1
#define ILLEGAL_IMMEDIATELY_CLOSE 0
galay::GY_TcpCoroutine<galay::CoroutineStatus> test(galay::GY_HttpController::wptr ctrl)
{
    auto request = ctrl.lock()->GetRequest();
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
    co_return galay::CoroutineStatus::kCoroutineFinished;
}

#if ILLEGAL_NOT_CLOSE
galay::GY_TcpCoroutine<galay::TaskStatus> illegal(std::string& rbuffer,std::string& wbuffer)
{
    wbuffer = "your protocol is illegal , please try again";
    co_return galay::TaskStatus::GY_TASK_SEND;
}
#elif ILLEGAL_DELAY_CLOSE
galay::GY_TcpCoroutine<galay::CoroutineStatus> illegal(std::string& rbuffer,std::string& wbuffer)
{
    wbuffer = "your protocol is illegal , please try again";
    co_return galay::CoroutineStatus::kCoroutineFinished;
}
#elif ILLEGAL_IMMEDIATELY_CLOSE
galay::GY_TcpCoroutine<galay::TaskStatus> illegal(std::string& rbuffer,std::string& wbuffer)
{
    wbuffer = "your protocol is illegal , please try again";
    co_return galay::TaskStatus::GY_TASK_IMMEDIATELY_CLOSE;
}
#endif

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
    router->Get("/echo",test);
    galay::GY_HttpServerBuilder::ptr builder = galay::GY_ServerBuilderFactory::CreateHttpServerBuilder(8082,router);
    server = galay::GY_ServerFactory::CreateHttpServer(builder);
    server->Start();
    return 0;
}