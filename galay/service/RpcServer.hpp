#ifndef GALAY_RPC_SERVER_HPP
#define GALAY_RPC_SERVER_HPP

#include "RpcThread.hpp"
#include <grpcpp/grpcpp.h> 
#include <memory>


namespace galay::service 
{

struct RpcServerConfig
{
    struct SSLConf {
        bool enable = false;
        grpc::string pem_root_certs;
        grpc::string pem_private_key;
        grpc::string pem_cert_chain;
    };


    SSLConf ssl_conf;
    int m_cq_threads = 1;
    //[min, max]
    std::pair<int, int> grpc_poller_threads = {1, 4};
};


class RpcFunctionHandler {
    friend class RpcService;
public:
    using uptr = std::unique_ptr<RpcFunctionHandler>;

    RpcFunctionHandler(grpc::Service* service, grpc::ServerCompletionQueue* cq)
        :m_service(service), m_cq(cq) {}
    virtual std::string Name() = 0;
    virtual void AsyncRequest(void* tag) = 0;
    virtual galay::Coroutine<void> HandleEvent() = 0;
    virtual void Finish(void *tag) = 0;
    virtual RpcFunctionHandler* NewFunctionCaller() = 0;
protected:
    grpc::Service* m_service;
    grpc::ServerCompletionQueue* m_cq;
    grpc::ServerContext m_context;
};

template<typename T>
concept ServiceType = requires()
{
    std::is_base_of_v<grpc::Service, T>;
};

template<typename SlefType, ServiceType T>
class RpcFunctionHandlerImpl: public RpcFunctionHandler
{
public:
    RpcFunctionHandlerImpl(grpc::Service* service, grpc::ServerCompletionQueue* cq)
        :RpcFunctionHandler(service, cq) {}

    T* GetService()
    {
        return static_cast<T*>(this->m_service);
    } 

    grpc::ServerCompletionQueue* GetCompletionQueue()
    {
        return this->m_cq;
    }

    grpc::ServerContext* GetServerContext()
    {
        return &this->m_context;
    }

    RpcFunctionHandler* NewFunctionCaller() override
    {
        return new SlefType(m_service, m_cq);
    }

private:
    using RpcFunctionHandler::m_service;
    using RpcFunctionHandler::m_cq;
    using RpcFunctionHandler::m_context;
};


class RpcService
{
public:
    using uptr = std::unique_ptr<RpcService>;

    RpcService(grpc::ServerCompletionQueue* cq, grpc::Service* service);

    template<typename T>
    T* NewCaller()
    {
        static_assert(
            std::is_base_of<RpcFunctionHandler, T>::value,
            "Template type T must inherit from RpcFunctionHandler"
        );
        auto caller = new T(m_service, m_cq);
        m_callers.push_back(caller);
        return caller;
    }

    void Start();

    ~RpcService();
private:
    grpc::ServerCompletionQueue* m_cq;
    grpc::Service* m_service;
    std::vector<RpcFunctionHandler*> m_callers;
};

class RpcServer final {
    using RpcServiceGroup = std::vector<std::unique_ptr<RpcService>>;
    using ThreadSelector = galay::details::RoundRobinLoadBalancer<RpcServerCQThread>;
public:
    explicit RpcServer(RpcServerConfig&& config);
    
    RpcService* CreateRpcService(grpc::Service* service, int thread_id = -1);

    bool Start(const std::string& server_address);

    void ResizeRunningThreads(int threads);
    void Stop();

private:
    grpc::ServerBuilder m_builder;
    RpcServiceGroup m_services;
    RpcServerConfig m_config;
    std::unique_ptr<grpc::Server> m_server;

    ThreadSelector::uptr m_selector;
    std::vector<RpcServerCQThread::uptr> m_threads;
};

}

#endif