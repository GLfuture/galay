#include <iostream>
#include "galay/galay.hpp"
#include "galay/service/RpcClient.hpp"
#include "hello.grpc.pb.h"  // 替换为你的生成文件路径

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using namespace galay::service;

class HelloClientCaller: public galay::service::RpcFunctionCallerImpl<HelloRequest, HelloResponse>
{
public:
    HelloClientCaller(std::shared_ptr<grpc::Channel> channel, grpc::CompletionQueue* cq)
        :RpcFunctionCallerImpl(channel, cq), m_stub(Hello::NewStub(channel))
    {
    }

    void AsyncCall(HelloRequest* request, HelloResponse* response, Status* status, void *tag) override
    {
        auto rpc = m_stub->AsyncSayHello(GetContext(), *request, GetCompletionQueue());
        rpc->Finish(response, status, tag);
    }
private:
    std::unique_ptr<Hello::Stub> m_stub;
};


galay::Coroutine<void> CallHello(galay::RoutineCtx ctx, RpcClient& client)
{
    Status status;
    HelloRequest request;
    HelloResponse response;
    auto caller = client.NewCaller<HelloClientCaller>();
    request.set_msg("Say");
    co_await client.Call<void>(caller.get(), &request, &response, &status);
    if (status.ok()) {
        std::cout << "CallHello: " << response.msg() << std::endl;
    } else {
        std::cout << "CallHello: " << status.error_code() << ": " << status.error_message() << std::endl;
    }
    co_return;
}

int main() {
    galay::GalayEnvConf gconf;
    gconf.m_coroutineSchedulerConf.m_thread_num = 1;
    galay::GalayEnv env(gconf);
    RpcClientConfig config("localhost:10001");
    RpcClient client(std::move(config));
    client.StartCQThread();
    CallHello(galay::RoutineCtx::Create(nullptr), client);
    getchar();
    client.StopCQThread();
    return 0;
}