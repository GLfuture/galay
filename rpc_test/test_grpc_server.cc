#include "galay/galay.hpp"
#include "galay/service/RpcServer.hpp"
#include "hello.grpc.pb.h"
#include <iostream>


using namespace galay::service;

class SayHelloCall: public RpcFunctionServerCallerImpl<SayHelloCall, Hello::AsyncService> {
public:
    SayHelloCall(grpc::Service* service, grpc::ServerCompletionQueue* m_cq) 
        : RpcFunctionServerCallerImpl(service, m_cq), m_responder(GetServerContext())
    {

    }

    std::string Name() {
        return "SayHelloCall";
    }

    void AsyncRequest(void* tag) override 
    {
        GetService()->RequestSayHello(GetServerContext(), &m_request, &m_responder, GetCompletionQueue(), GetCompletionQueue(), tag);
    }

    galay::Coroutine<void> HandleEvent() override
    {
        return HandleEventImpl(galay::RoutineCtx::Create(nullptr), m_request, m_response);
    }

    void Finish(void *tag) override
    {
        m_responder.Finish(m_response, grpc::Status::OK, tag);
    }
private:
    static galay::Coroutine<void> HandleEventImpl(galay::RoutineCtx ctx, HelloRequest& request, HelloResponse& response)
    {
        response.set_msg("Hello" + request.msg());
        co_return;
    }
private:
    HelloRequest m_request;
    HelloResponse m_response;
    grpc::ServerAsyncResponseWriter<HelloResponse> m_responder;
};

class SayWorldCall: public RpcFunctionServerCallerImpl<SayWorldCall, Hello::AsyncService> {
public:
    SayWorldCall(grpc::Service* service, grpc::ServerCompletionQueue* m_cq) 
        : RpcFunctionServerCallerImpl(service, m_cq), m_responder(GetServerContext())
    {

    }

    std::string Name()
    {
        return "SayWorldCall";
    }

    void AsyncRequest(void* tag) override
    {
        GetService()->RequestSayWorld(GetServerContext(), &m_request, &m_responder, GetCompletionQueue(), GetCompletionQueue(), tag);
    }

    galay::Coroutine<void> HandleEvent() override
    {
        return HandleEventImpl(galay::RoutineCtx::Create(nullptr), m_request, m_response);
    }

    void Finish(void *tag) override
    {
        m_responder.Finish(m_response, grpc::Status::OK, tag);
    }

    
private:
    static galay::Coroutine<void> HandleEventImpl(galay::RoutineCtx ctx, WorldRequest& request, WorldResponse& response)
    {
        response.set_msg("World" + request.msg());
        co_return;
    }

private:
    WorldRequest m_request;
    WorldResponse m_response;
    grpc::ServerAsyncResponseWriter<WorldResponse> m_responder;
};

class ExitCall: public RpcFunctionServerCallerImpl<ExitCall, System::AsyncService> {
public:
    ExitCall(grpc::Service* service, grpc::ServerCompletionQueue* m_cq) 
        : RpcFunctionServerCallerImpl(service, m_cq), m_responder(GetServerContext())
    {

    }

    std::string Name() override
    {
        return "ExitCall";
    }

    void AsyncRequest(void* tag) override
    {
        GetService()->RequestExit(GetServerContext(), &m_request, &m_responder, GetCompletionQueue(), GetCompletionQueue(), tag);
    }

    galay::Coroutine<void> HandleEvent() override
    {
        return HandleEventImpl(galay::RoutineCtx::Create(nullptr), m_request, m_response);
    }

    void Finish(void *tag) override
    {
        m_responder.Finish(m_response, grpc::Status::OK, tag);
    }


private:
    static galay::Coroutine<void> HandleEventImpl(galay::RoutineCtx ctx, ExitRequest& request, ExitResponse& response)
    {
        response.set_code(request.code());
        co_return;
    }

private:
    ExitRequest m_request;
    ExitResponse m_response;
    grpc::ServerAsyncResponseWriter<ExitResponse> m_responder;
};


int main()
{
    galay::GalayEnvConf gconf;
    gconf.m_coroutineSchedulerConf.m_thread_num = 1;
    galay::GalayEnv env(gconf);
    auto hello = new Hello::AsyncService;
    auto exit = new System::AsyncService;
    RpcServerConfig conf;
    RpcServer server(std::move(conf));
    auto rpc_service = server.CreateRpcService(hello);
    rpc_service->NewCaller<SayHelloCall>();
    rpc_service->NewCaller<SayWorldCall>();
    auto exit_service = server.CreateRpcService(exit);
    exit_service->NewCaller<ExitCall>();
    server.Start("0.0.0.0:10001");
    getchar();
    server.Stop();
    return 0;
}