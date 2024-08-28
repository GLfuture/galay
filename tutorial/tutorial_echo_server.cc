#include "../galay/galay.h"
#include <condition_variable>
#include <spdlog/spdlog.h>
#include <mutex>
#include <signal.h>
#include <iostream>

class Business : public galay::HttpCoreBase
{
public:
    NetCoroutine CoreBusiness(HttpController::ptr ctrl) override
    {
        auto request = ctrl->GetRequest();
        auto response = std::make_shared<galay::protocol::http::HttpResponse>();
        response->Header()->Code() = 200;
        response->Header()->Version() = "1.1";
        response->Header()->HeaderPairs().AddHeaderPair("Content-Type", "text/html");
        std::string body = "<html><head><meta charset=\"utf-8\"><title>title</title></head><body>hello world!</body></html>";
        response->Body() = std::move(body);
        auto res = ctrl->Send(response->EncodePdu());
        ctrl->AddTimer(10,2,[](galay::poller::Timer::ptr timer){
            std::cout << "timer" << std::endl;
        });
        co_await res.Wait();
        std::cout << request->Error()->ToString(request->Error()->Code()) << '\n';
        co_return galay::coroutine::CoroutineStatus::kCoroutineFinished;
    }
};

class FormsBusiness : public galay::HttpCoreBase
{
public:
    NetCoroutine CoreBusiness(HttpController::ptr ctrl) override
    {
        auto request = ctrl->GetRequest();
        auto response = std::make_shared<galay::protocol::http::HttpResponse>();
        response->Header()->Code() = 200;
        response->Header()->Version() = "1.1";
        response->Header()->HeaderPairs().AddHeaderPair("Content-Type", "text/html");
        std::string body = "<html><head><meta charset=\"utf-8\"><title>title</title></head><body>success</body></html>";
        response->Body() = std::move(body);
        auto res = ctrl->Send(response->EncodePdu());
        co_await res.Wait();
        std::cout << request->Error()->ToString(request->Error()->Code()) << '\n';
        co_return galay::coroutine::CoroutineStatus::kCoroutineFinished;
    }
};

class ChunckBusiness : public galay::HttpCoreBase
{
public:
    NetCoroutine CoreBusiness(HttpController::ptr ctrl) override
    {
        auto request = ctrl->GetRequest();
        request->StartChunck();
        auto res = ctrl->Send(request->EncodePdu());
        for(int i = 0; i < 20; i++)
        {
            ctrl->Send(request->ChunckStream(std::to_string(i)));
        }
        res = ctrl->Send(request->EndChunck());
        co_await res.Wait();
        co_return galay::coroutine::CoroutineStatus::kCoroutineFinished;
    }
};

galay::coroutine::NetCoroutine WrongHttpHandle(galay::server::HttpController::ptr ctrl)
{
    auto request = ctrl->GetRequest();
    auto err = request->Error();
    if(err->HasError()) {
        std::cout << err->ToString(err->Code()) << '\n';
    }
    galay::protocol::http::HttpResponse response;
    response.Header()->Code() = 400;
    response.Header()->Version() = "1.1";
    response.Header()->HeaderPairs().AddHeaderPair("Content-Type", "text/html");
    std::string body = "<html><head><meta charset=\"utf-8\"><title>title</title></head><body>400 Bad Request</body></html>";
    response.Body() = std::move(body);
    auto res = ctrl->Send(response.EncodePdu());
    co_await res.Wait();
    co_return galay::coroutine::CoroutineStatus::kCoroutineFinished;
}

int main()
{
    spdlog::set_level(spdlog::level::debug);
    Business business;
    ChunckBusiness chunckbusiness;
    FormsBusiness formsbusiness;
    auto router = galay::RouterFactory::CreateHttpRouter();
    router->Get("/echo",std::bind(&Business::CoreBusiness,&business,std::placeholders::_1));
    router->Get("/chuncked",std::bind(&ChunckBusiness::CoreBusiness,&chunckbusiness,std::placeholders::_1));
    router->Post("/forms",std::bind(&FormsBusiness::CoreBusiness,&formsbusiness,std::placeholders::_1));
    router->RegisterWrongHandle(WrongHttpHandle);
    galay::server::HttpServerBuilder::ptr builder = galay::ServerBuilderFactory::CreateHttpServerBuilder(8899,router);
    builder->SetNetThreadNum(4);
    galay::server::TcpServer::ptr server = galay::ServerFactory::CreateHttpServer(builder);
    server->Start();
    return 0;
}  