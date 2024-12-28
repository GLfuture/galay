#ifndef GALAY_ETCD_HPP
#define GALAY_ETCD_HPP

#include <etcd/Client.hpp>
#include <etcd/KeepAlive.hpp>
#include <etcd/Watcher.hpp> 
#include <shared_mutex>
#include <unordered_map>
#include "galay/kernel/Coroutine.hpp"

namespace galay::details
{
class EtcdAction: public WaitAction
{
public:
    virtual bool DoAction(CoroutineBase::wptr co, void* ctx) override;
    inline virtual bool HasEventToDo() override { return true; }
    void ResetCallback(std::function<void(CoroutineBase::wptr, void*)>&& callback) { m_callback = std::move(callback); }
private:
    std::function<void(CoroutineBase::wptr, void*)> m_callback;
};

}

namespace galay::etcd
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
        template<typename CoRtn>
        AsyncResult<bool, CoRtn> RegisterService(const std::string& ServiceName, const std::string& ServiceAddr);
        template<typename CoRtn>
        AsyncResult<bool, CoRtn> RegisterService(const std::string& ServiceName, const std::string& ServiceAddr, int TTL);
        template<typename CoRtn>
        AsyncResult<bool, CoRtn> DiscoverService(const std::string& ServiceName);
        template<typename CoRtn>
        AsyncResult<bool, CoRtn> DiscoverServicePrefix(const std::string& Prefix);
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
        details::EtcdAction m_action;
        std::shared_ptr<::etcd::Client> m_client;
        std::shared_ptr<::etcd::KeepAlive> m_keepalive;
        std::shared_ptr<::etcd::Watcher> m_watcher;
    };
}

#include "Etcd.tcc"
#endif