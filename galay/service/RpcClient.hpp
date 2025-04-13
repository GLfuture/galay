#ifndef GALAY_RPC_CLIENT_HPP
#define GALAY_RPC_CLIENT_HPP

#include "RpcThread.hpp"
#include <grpcpp/channel.h>

namespace galay::service
{

struct RpcClientConfig
{
    struct SSLConf {
        bool enable = false;
        grpc::string pem_root_certs;

        bool enable_mtls = false;
        grpc::string pem_cert_chain;
        grpc::string pem_private_key;
    };
    std::string server_address;
    SSLConf ssl_conf;

    RpcClientConfig(const std::string& server_address)
        :server_address(server_address)
    {
    }
};

class RpcFunctionClientCaller
{
public:
    RpcFunctionClientCaller(std::shared_ptr<grpc::Channel> channel, grpc::CompletionQueue* cq)
        :m_channel(channel), m_cq(cq)
    {
    }
    virtual void AsyncCall(grpc::Status* status, void *tag) = 0;
protected:
    grpc::ClientContext m_context;
    std::shared_ptr<grpc::Channel> m_channel;
    grpc::CompletionQueue* m_cq;
};

template<typename SlefType>
class RpcFunctionClientCallerImpl: public RpcFunctionClientCaller
{
public:
    RpcFunctionClientCallerImpl(std::shared_ptr<grpc::Channel> channel, grpc::CompletionQueue* cq)
        :RpcFunctionClientCaller(channel, cq)
    {
    }

    grpc::ClientContext* GetContext()
    {
        return &m_context;
    }

    grpc::Channel* GetChannel()
    {
        return m_channel.get();
    }

    grpc::CompletionQueue* GetCompletionQueue()
    {
        return m_cq;
    }
    
private:
    using RpcFunctionClientCaller::m_channel;
    using RpcFunctionClientCaller::m_context;
};


class RpcClient final
{
public:

    //如果thread为空，内部自动创建并启动（not call start）；非空（not call start）
    RpcClient(RpcClientConfig&& config, RpcClientCQThread::ptr thread = nullptr)
        :m_config(std::move(config))
    {
        if(!m_config.ssl_conf.enable) {
            m_channel = grpc::CreateChannel(m_config.server_address, grpc::InsecureChannelCredentials());
        } else {
            grpc::SslCredentialsOptions options;
            options.pem_root_certs = m_config.ssl_conf.pem_root_certs;
            if (m_config.ssl_conf.enable_mtls) {
                // 加载客户端证书和私钥
                options.pem_private_key = m_config.ssl_conf.pem_private_key;
                options.pem_cert_chain = m_config.ssl_conf.pem_cert_chain;
            }

            auto cred = grpc::SslCredentials(options);
            grpc::ChannelArguments args;
            args.SetSslTargetNameOverride(m_config.server_address);
            m_channel = grpc::CreateCustomChannel(
                m_config.server_address,
                cred,
                args
            );
        }

        if (m_channel->GetState(false) == GRPC_CHANNEL_TRANSIENT_FAILURE) {
            throw std::runtime_error("Channel connection failed during initialization");
            exit(-1);
        }

        if(thread) {
            m_thread = thread;
        } else {
            auto cq = std::make_unique<grpc::CompletionQueue>();
            m_thread = std::make_shared<RpcClientCQThread>(std::move(cq));
        }
    }

    template<typename T>
    std::unique_ptr<T> NewCaller()
    {
        static_assert(
            std::is_base_of<RpcFunctionClientCaller, T>::value,
            "Template type T must inherit from RpcFunctionClientCaller"
        );
        return std::make_unique<T>(m_channel, m_thread->GetCompletionQueue());
    }

    template<typename CoRtn>
    AsyncResult<void, CoRtn> Call(RpcFunctionClientCaller* caller, grpc::Status* status)
    {
        return this_coroutine::WaitAsyncRtnExecute<void>(OnceCall(galay::RoutineCtx::Create(nullptr), caller, status));
    }

    void StartCQThread()
    {
        m_thread->Start();
    }

    void StopCQThread()
    {
        m_thread->Stop();
    }

private:
    static galay::Coroutine<void> OnceCall(galay::RoutineCtx ctx, RpcFunctionClientCaller* caller, grpc::Status* status)
    {
        auto co = co_await this_coroutine::GetThisCoroutine<void>();
        std::cout << (void*)co.lock().get() << std::endl;
        caller->AsyncCall(status, co.lock().get());
        co_yield {};
        co_return;
    }

private:
    RpcClientConfig m_config;
    std::shared_ptr<::grpc::Channel> m_channel;
    RpcClientCQThread::ptr m_thread;
};




}
#endif