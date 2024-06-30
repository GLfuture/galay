#include "../galay/galay.h"
#include <condition_variable>
#include <spdlog/spdlog.h>
#include <mutex>
#include <signal.h>
#include <iostream>

class Business : public galay::GY_HttpCoreBase
{
public:
    GY_TcpCoroutine CoreBusiness(GY_HttpController::wptr ctrl) override
    {
        auto request = ctrl.lock()->GetRequest();
        auto response = std::make_shared<galay::protocol::http::Http1_1_Response>();
        response->SetStatus(200);
        response->SetVersion("1.1");
        response->SetHeadPair({"Content-Type", "text/html"});
        std::string body = "<html><head><meta charset=\"utf-8\"><title>title</title></head><body>hello world!</body></html>";
        response->SetBody(std::move(body));
        ctrl.lock()->PushResponse(response->EncodePdu());
        galay::kernel::WaitGroup group;
        group.Add(1);
        // 模拟耗时任务
        std::thread th([ctrl, &group]()
                       {
        std::cout << "start ....\n";
        std::this_thread::sleep_for(std::chrono::seconds(5));
        if(!ctrl.expired()) ctrl.lock()->PushResponse("end....\n");
        group.Done(); });
        // 一定要detach,不能是join，否则线程内唤醒协程会死锁
        th.detach();
        std::cout << "wait....\n";
        co_await group.Wait();
        if (request->GetHeadValue("connection").compare("close") == 0)
        {
            ctrl.lock()->Close();
        }
        ctrl.lock()->Done();
        co_return galay::common::CoroutineStatus::kCoroutineFinished;
    }

private:

};


int main()
{
    spdlog::set_level(spdlog::level::debug);
    Business business;
    auto router = galay::GY_RouterFactory::CreateHttpRouter();
    router->Get("/echo",std::bind(&Business::CoreBusiness,&business,std::placeholders::_1));
    galay::kernel::GY_HttpServerBuilder::ptr builder = galay::GY_ServerBuilderFactory::CreateHttpServerBuilder(8082,router);
    galay::kernel::GY_TcpServer::ptr server = galay::GY_ServerFactory::CreateHttpServer(builder);
    
    server->Start();
    return 0;
}