#ifndef GALAY_RPCTHREAD_HPP
#define GALAY_RPCTHREAD_HPP

#include "galay/kernel/ExternApi.hpp"
#include <grpcpp/grpcpp.h>
#include <thread>

namespace galay::service
{
class RpcServerCQThread final {
public:
    using ptr = std::shared_ptr<RpcServerCQThread>;
    using uptr = std::unique_ptr<RpcServerCQThread>;

    RpcServerCQThread(std::unique_ptr<grpc::ServerCompletionQueue> cq);

    void Start();
    void Stop();

    grpc::ServerCompletionQueue* GetCompletionQueue();
private:
    void HandleRpcs();
private:
    std::thread m_thread;
    std::unique_ptr<grpc::ServerCompletionQueue> m_cq;
};


class RpcClientCQThread final {
public:
    using ptr = std::shared_ptr<RpcClientCQThread>;
    using uptr = std::unique_ptr<RpcClientCQThread>;

    RpcClientCQThread(std::unique_ptr<grpc::CompletionQueue> cq);

    void Start();
    void Stop();

    grpc::CompletionQueue* GetCompletionQueue();
private:
    void HandleRpcs();

private:
    std::thread m_thread;
    std::unique_ptr<grpc::CompletionQueue> m_cq;
};


}


#endif