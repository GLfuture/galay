#ifndef GALAY_ETCD_H
#define GALAY_ETCD_H

#include <memory>
#include <string>
#include <etcd/Client.hpp>
#include <etcd/KeepAlive.hpp>
#include <etcd/Watcher.hpp>
#include <coroutine>
#include <unordered_map>
#include <functional>
#include <shared_mutex>
#include <any>
#include "../common/result.h"

namespace galay::middleware::etcd
{
    class ServiceCenter
    {
    public:
        static ServiceCenter* GetInstance();

        std::string GetServiceAddr(const std::string& ServiceName);
        void SetServiceAddr(const std::string& ServiceName, const std::string& ServiceAddr);
    public:
        static std::unique_ptr<ServiceCenter> m_instance;
        std::shared_mutex m_mutex;
        std::unordered_map<std::string, std::string> m_serviceAddr;
    };

    class EtcdResult
    {
    public:
        EtcdResult(result::ResultInterface::ptr result);
        //DiscoverService
        std::string ServiceAddr();
        //DiscoverServicePrefix
        std::vector<std::pair<std::string, std::string>> ServiceAddrs();
        bool Success();
        std::string Error();
        coroutine::GroupAwaiter& Wait();
    private:
        result::ResultInterface::ptr m_result;
    };

    class EtcdClient
    {
    public:
        EtcdClient(const std::string& EtcdAddrs);
        EtcdResult RegisterService(const std::string& ServiceName, const std::string& ServiceAddr);
        EtcdResult RegisterService(const std::string& ServiceName, const std::string& ServiceAddr, int TTL);
        EtcdResult DiscoverService(const std::string& ServiceName);
        EtcdResult DiscoverServicePrefix(const std::string& Prefix);

        //监视一个key
        void Watch(const std::string& key, std::function<void(::etcd::Response)> handle);
        void CancleWatch();
        //分布式锁
        void Lock(const std::string& key);
        void Lock(const std::string& key , int TTL);
        void UnLock();
    private:
    
        bool CheckExist(const std::string& key);
    private:
        std::shared_ptr<::etcd::Client> m_client;
        std::shared_ptr<::etcd::KeepAlive> m_keepalive;
        std::shared_ptr<::etcd::Watcher> m_watcher;
        std::string m_lockKey;
    };
}


#endif