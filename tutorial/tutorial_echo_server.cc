#include "../galay/galay.h"
#include <condition_variable>
#include <spdlog/spdlog.h>
#include <mutex>
#include <signal.h>
#include <iostream>
galay::common::GY_TcpCoroutine<galay::common::CoroutineStatus> test(galay::kernel::GY_HttpController::wptr ctrl)
{
    auto request = ctrl.lock()->GetRequest();
    auto response = std::make_shared<galay::protocol::http::Http1_1_Response>();
    response->SetStatus(200) ;
    response->SetVersion("1.1");
    response->SetHeadPair({"Content-Type", "text/html"});
    std::string body = "<html><head><meta charset=\"utf-8\"><title>title</title></head><body>hello world!</body></html>";
    response->SetBody(std::move(body));
    ctrl.lock()->PushResponse(response->EncodePdu());
    galay::kernel::WaitGroup group;
    group.Add(1);
    //模拟耗时任务
    std::thread th([ctrl,&group](){
        std::cout << "start ....\n";
        std::this_thread::sleep_for(std::chrono::seconds(5));
        if(!ctrl.expired()) ctrl.lock()->PushResponse("end....\n");
        group.Done();
    });
    //一定要detach,不能是join，否则线程内唤醒协程会死锁
    th.detach();
    std::cout << "wait....\n";
    co_await group.Wait();
    if(request->GetHeadValue("connection").compare("close") == 0){
        ctrl.lock()->Close();
    }
    ctrl.lock()->Done();
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
    router->Get("/echo",test);
    galay::kernel::GY_HttpServerBuilder::ptr builder = galay::GY_ServerBuilderFactory::CreateHttpServerBuilder(8082,router);
    server = galay::GY_ServerFactory::CreateHttpServer(builder);
    server->Start();
    return 0;
}