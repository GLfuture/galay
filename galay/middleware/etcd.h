#ifndef GALAY_ETCD_H
#define GALAY_ETCD_H

#include <etcd/Client.hpp>
#include <etcd/KeepAlive.hpp>
#include <etcd/Watcher.hpp> 
#include "../util/ThreadSecurity.hpp"

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
        thread::security::SecurityHashMap<std::string,std::string> m_serviceAddr;
    };

    class EtcdClient
    {
    public:
        EtcdClient(const std::string& EtcdAddrs);
        EtcdResult RegisterService(const std::string& ServiceName, const std::string& ServiceAddr);
        EtcdResult RegisterService(const std::string& ServiceName, const std::string& ServiceAddr, int TTL);
        EtcdResult DiscoverService(const std::string& ServiceName);
        EtcdResult DiscoverServicePrefix(const std::string& Prefix);
        std::string GetLastError();
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