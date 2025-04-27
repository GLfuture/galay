#include "RpcServer.hpp"
#include <grpcpp/impl/service_type.h> 
#include <grpcpp/server_builder.h>
#include <grpcpp/completion_queue.h>
#include <grpcpp/server_context.h>

namespace galay::service
{

galay::Coroutine<void> HandleFunctionCall(galay::RoutineCtx ctx, std::unique_ptr<RpcFunctionHandler> caller)
{
    auto co = co_await galay::this_coroutine::GetThisCoroutine<void>();
    caller->AsyncRequest(co.lock().get());
    co_yield {};
    HandleFunctionCall(galay::RoutineCtx::Create(nullptr), std::unique_ptr<RpcFunctionHandler>(caller->NewFunctionCaller()));
    co_await galay::this_coroutine::WaitAsyncRtnExecute<void>(caller->HandleEvent());
    caller->Finish(co.lock().get());
    co_yield {};
    co_return;
}

RpcService::RpcService(grpc::ServerCompletionQueue *cq, grpc::Service* service)
    :m_cq(cq), m_service(service)
{
}

void RpcService::Start()
{
    assert(m_cq != nullptr);
    for(auto& caller: m_callers) {
        HandleFunctionCall(galay::RoutineCtx::Create(nullptr), std::unique_ptr<RpcFunctionHandler>(caller));
    }
    m_callers.clear();
}

RpcService::~RpcService()
{
    delete m_service;
    for(auto& caller: m_callers) {
        delete caller;
    }
}

RpcServer::RpcServer(RpcServerConfig &&config)
: m_config(std::move(config)) 
{
    grpc::EnableDefaultHealthCheckService(true);
    ResizeRunningThreads(m_config.m_cq_threads);
}

RpcService *RpcServer::CreateRpcService(grpc::Service *service, int thread_id)
{
    if(m_threads.size() == 0) return nullptr; 
    m_builder.RegisterService(service);
    if(thread_id >= 0) m_services.emplace_back(std::make_unique<RpcService>(m_threads[thread_id]->GetCompletionQueue(), std::move(service)));
    else {
        m_services.emplace_back(std::make_unique<RpcService>(m_selector->Select()->GetCompletionQueue(), std::move(service)));
    }
    return m_services.back().get();
}

bool RpcServer::Start(const std::string &server_address)
{
    int port;
    if(!m_config.ssl_conf.enable) {
        m_builder.AddListeningPort(server_address, grpc::InsecureServerCredentials(), &port);
    } else {
        grpc::SslServerCredentialsOptions options;
        options.pem_key_cert_pairs = {{m_config.ssl_conf.pem_private_key, m_config.ssl_conf.pem_cert_chain}};
        options.pem_root_certs = m_config.ssl_conf.pem_root_certs;
        m_builder.AddListeningPort(server_address, grpc::SslServerCredentials(options), &port);
    }
    m_builder.SetSyncServerOption(
        grpc::ServerBuilder::MIN_POLLERS, m_config.grpc_poller_threads.first
    );
    m_builder.SetSyncServerOption(
        grpc::ServerBuilder::MAX_POLLERS, m_config.grpc_poller_threads.second
    );
    m_server = m_builder.BuildAndStart();
    for(auto &service: m_services) {
        service->Start();
    }
    for(auto &thread : m_threads) {
        thread->Start(); 
    }
    return true;
}

void RpcServer::ResizeRunningThreads(int threads)
{
    for(auto &thread : m_threads) {
        thread->Stop();
    }
    m_threads.clear();
    std::vector<RpcServerCQThread*> vec(threads);
    for(int i = 0; i < threads; ++i) {
        auto th = std::make_unique<RpcServerCQThread>(m_builder.AddCompletionQueue());
        vec[i] = th.get();
        m_threads.push_back(std::move(th));
    }
    m_selector = std::make_unique<ThreadSelector>(vec);
}

void RpcServer::Stop()
{
    m_server->Shutdown();
    for(auto& thread : m_threads) {
        thread->Stop();
    }
}


}