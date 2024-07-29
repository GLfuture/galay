#include "../galay/galay.h"
#include <condition_variable>
#include <spdlog/spdlog.h>
#include <mutex>
#include <signal.h>
#include <iostream>

class Business : public galay::GY_HttpCoreBase
{
public:
    GY_NetCoroutine CoreBusiness(GY_HttpController::wptr ctrl) override
    {
        auto request = ctrl.lock()->GetRequest();
        auto response = std::make_shared<galay::protocol::http::HttpResponse>();
        response->Header()->Code() = 200;
        response->Header()->Version() = "1.1";
        response->Header()->Headers()["Content-Type"] = "text/html";
        std::string body = "<html><head><meta charset=\"utf-8\"><title>title</title></head><body>hello world!</body></html>";
        response->Body() = std::move(body);
        galay::common::WaitGroup group(ctrl.lock()->GetCoPool());
        group.Add(1);
        // 模拟耗时任务
        std::thread th([ctrl, &group]()
        {
            std::cout << "start ....\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            group.Done(); 
        });
        // 一定要detach,不能是join，否则线程内唤醒协程会死锁
        th.detach();
        std::cout << "wait....\n";
        co_await group.Wait();
        if (request->Header()->Headers().contains("connection")&& request->Header()->Headers()["connection"].compare("close") == 0)
        {
            ctrl.lock()->Close();
        }
        std::cout << "end....\n";
        //Close 要在 PushResponse 前，不然失效
        ctrl.lock()->PushResponse(response->EncodePdu());
        ctrl.lock()->Done();
        co_return galay::common::CoroutineStatus::kCoroutineFinished;
    }

private:

};

class ChunckBusiness : public galay::GY_HttpCoreBase
{
public:
    GY_NetCoroutine CoreBusiness(GY_HttpController::wptr ctrl) override
    {
        auto request = ctrl.lock()->GetRequest();
        request->StartChunck();
        ctrl.lock()->PushResponse(request->EncodePdu());
        for(int i = 0; i < 20; i++)
        {
            ctrl.lock()->PushResponse(request->ChunckStream(std::to_string(i)));
        }
        ctrl.lock()->PushResponse(request->EndChunck());
        sleep(3);
        ctrl.lock()->Done();
        co_return galay::common::CoroutineStatus::kCoroutineFinished;
    }

private:

};

int main()
{
    spdlog::set_level(spdlog::level::info);
    Business business;
    ChunckBusiness chunckbusiness;
    auto router = galay::GY_RouterFactory::CreateHttpRouter();
    router->Get("/echo",std::bind(&Business::CoreBusiness,&business,std::placeholders::_1));
    router->Get("/chuncked",std::bind(&ChunckBusiness::CoreBusiness,&chunckbusiness,std::placeholders::_1));
    galay::kernel::GY_HttpServerBuilder::ptr builder = galay::GY_ServerBuilderFactory::CreateHttpServerBuilder(8899,router);
    builder->SetIllegalFunction([]()->std::string{
        galay::protocol::http::HttpResponse response;
        response.Header()->Code() = 400;
        response.Header()->Version() = "1.1";
        response.Header()->Headers()["Content-Type"] = "text/html";
        std::string body = "<html><head><meta charset=\"utf-8\"><title>title</title></head><body>400 Bad Request</body></html>";
        response.Body() = std::move(body);
        return response.EncodePdu();
    });
    builder->SetNetThreadNum(4);
    galay::kernel::GY_TcpServer::ptr server = galay::GY_ServerFactory::CreateHttpServer(builder);
    server->Start();
    return 0;
}