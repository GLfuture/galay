#ifndef GALAY_SERVICE_HPP
#define GALAY_SERVICE_HPP

#include "galay/kernel/Coroutine.hpp"
#include <grpcpp/grpcpp.h>
#include <grpcpp/completion_queue.h>

namespace galay::details
{

class RpcWaitAction final: public WaitAction
{
public:
    RpcWaitAction();
    bool HasEventToDo() override;
    bool DoAction(CoroutineBase::wptr co, void* ctx) override;
private:
    
};

};

namespace galay::service 
{

using namespace grpc;

struct RcpServerConfig
{
    RcpServerConfig(const std::string ip, uint16_t port)
        : ip(ip), port(port) {}

    
    std::string ip;
    uint16_t port;
    uint16_t m_new_call_cq_size = 1;
    uint16_t m_notify_cq_size = 1; 
};

class CallData
{
public:

    Coroutine<void> HandleRpc(CallData* call_data);

};

class RpcServer
{
public:
    RpcServer(RcpServerConfig config);
    void Start();
private:

private:
    std::vector<std::unique_ptr<ServerCompletionQueue>> m_new_call_cq;     // 接收新请求
    std::vector<std::unique_ptr<ServerCompletionQueue>> m_notification_cq; // 处理事件
};


}

#endif