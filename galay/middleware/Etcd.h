#ifndef GALAY_ETCD_H
#define GALAY_ETCD_H

#include <etcd/Client.hpp>
#include <etcd/KeepAlive.hpp>
#include <etcd/Watcher.hpp> 
#include <shared_mutex>
#include <unordered_map>
#include "../kernel/Awaiter.h"

namespace galay::coroutine
{
    class Awaiter_bool;
    class Awaiter_string;
}

namespace galay::action
{
    class EtcdAction: public WaitAction
    {
    public:
        virtual bool DoAction(coroutine::Coroutine* co, void* ctx) override;
        inline virtual bool HasEventToDo() override { return true; }
        void ResetCallback(std::function<void(coroutine::Coroutine*, void*)>&& callback) { m_callback = std::forward<std::function<void(coroutine::Coroutine*,void*)>>(callback); }
    private:
        std::function<void(coroutine::Coroutine*, void*)> m_callback;
    };
}

namespace galay::middleware::etcd
{
    class EtcdResponse
    {
    public:
        EtcdResponse(::etcd::Response& response);
        bool Success();
        std::string GetValue();
        std::vector<std::pair<std::string,std::string>> GetKeyValues();
    private:
        ::etcd::Response& m_response;
    };

    class EtcdClient
    {
    public:
        EtcdClient(const std::string& EtcdAddrs, int co_sche_index);
        coroutine::Awaiter_bool RegisterService(const std::string& ServiceName, const std::string& ServiceAddr);
        coroutine::Awaiter_bool RegisterService(const std::string& ServiceName, const std::string& ServiceAddr, int TTL);
        coroutine::Awaiter_bool DiscoverService(const std::string& ServiceName);
        coroutine::Awaiter_bool DiscoverServicePrefix(const std::string& Prefix);
        EtcdResponse GetResponse();
        //监视一个key
        void Watch(const std::string& key, std::function<void(::etcd::Response)> handle);
        void CancleWatch();
        //分布式锁
        void Lock(const std::string& key);
        void Lock(const std::string& key , int TTL);
        void UnLock(const std::string& key); 
    private:
        bool CheckExist(const std::string& key);
    private:
        int m_co_sche_index;
        ::etcd::Response m_response;
        action::EtcdAction m_action;
        std::shared_ptr<::etcd::Client> m_client;
        std::shared_ptr<::etcd::KeepAlive> m_keepalive;
        std::shared_ptr<::etcd::Watcher> m_watcher;
    };
}


#endif