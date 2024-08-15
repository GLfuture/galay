#include "../galay/galay.h"
#include <condition_variable>
#include <spdlog/spdlog.h>
#include <mutex>
#include <signal.h>
#include <iostream>

class Business : public galay::GY_HttpCoreBase
{
public:
    GY_NetCoroutine CoreBusiness(GY_HttpController::ptr ctrl) override
    {
        auto request = ctrl->GetRequest();
        auto response = std::make_shared<galay::protocol::http::HttpResponse>();
        response->Header()->Code() = 200;
        response->Header()->Version() = "1.1";
        response->Header()->Headers()["Content-Type"] = "text/html";
        std::string body = "<html><head><meta charset=\"utf-8\"><title>title</title></head><body>hello world!</body></html>";
        response->Body() = std::move(body);
        auto res = ctrl->Send(response->EncodePdu());
        co_await res->Wait();
        ctrl->Close();
        co_return galay::coroutine::CoroutineStatus::kCoroutineFinished;
    }

private:

};

class ChunckBusiness : public galay::GY_HttpCoreBase
{
public:
    GY_NetCoroutine CoreBusiness(GY_HttpController::ptr ctrl) override
    {
        auto request = ctrl->GetRequest();
        request->StartChunck();
        auto res = ctrl->Send(request->EncodePdu());
        for(int i = 0; i < 20; i++)
        {
            ctrl->Send(request->ChunckStream(std::to_string(i)));
        }
        res = ctrl->Send(request->EndChunck());
        co_await res->Wait();
        co_return galay::coroutine::CoroutineStatus::kCoroutineFinished;
    }

private:

};

int main()
{
    spdlog::set_level(spdlog::level::debug);
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