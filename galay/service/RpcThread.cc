#include "RpcThread.hpp"

namespace galay::service 
{
RpcServerCQThread::RpcServerCQThread(std::unique_ptr<grpc::ServerCompletionQueue> cq)
    :m_cq(std::move(cq))
{
}

void RpcServerCQThread::Start()
{
    m_thread = std::thread(&RpcServerCQThread::HandleRpcs, this);
}

void RpcServerCQThread::Stop()
{
    m_cq->Shutdown();
    if (m_thread.joinable()) m_thread.join();
}

grpc::ServerCompletionQueue *RpcServerCQThread::GetCompletionQueue()
{
    return m_cq.get();
}

void RpcServerCQThread::HandleRpcs()
{
    void* tag;
    bool ok;
    while (m_cq->Next(&tag, &ok)) {
        if (ok) {
            auto co = static_cast<galay::Coroutine<void>*>(tag);
            co->GetCoScheduler()->ToResumeCoroutine(co->weak_from_this()) ;
        }
    }
}

RpcClientCQThread::RpcClientCQThread(std::unique_ptr<grpc::CompletionQueue> cq)
    : m_cq(std::move(cq))
{
}

void RpcClientCQThread::Start()
{
    m_thread = std::thread(&RpcClientCQThread::HandleRpcs, this);
}

void RpcClientCQThread::Stop()
{
    m_cq->Shutdown();
    if (m_thread.joinable()) m_thread.join();
}

grpc::CompletionQueue *RpcClientCQThread::GetCompletionQueue()
{
    return m_cq.get();
}


void RpcClientCQThread::HandleRpcs()
{
    void* tag;
    bool ok;
    while (m_cq->Next(&tag, &ok)) {
        if (ok) {
            auto co = static_cast<galay::Coroutine<void>*>(tag);
            co->GetCoScheduler()->ToResumeCoroutine(co->weak_from_this()) ;
        }
    }
}
}