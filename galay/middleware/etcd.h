#ifndef GALAY_ETCD_H
#define GALAY_ETCD_H

#include <memory>
#include <string>
#include <etcd/Client.hpp>
#include <etcd/KeepAlive.hpp>
#include <coroutine>
#include <unordered_map>
#include <functional>
#include <any>

namespace galay::middleware::etcd
{
    class EtcdAwaiter
    {
    private:
        std::function<void(std::coroutine_handle<>,std::any&)> m_Func;
        std::any m_Result;
    public:
        EtcdAwaiter(std::function<void(std::coroutine_handle<>,std::any&)>&& func);
        bool await_ready();
        void await_suspend(std::coroutine_handle<> handle);
        std::any await_resume();
    };

    //服务注册example /XXX/XXX/ServiceName/nodeX   ip:port
    //服务注册 (async)
    class ServiceRegister
    {
    private:
        std::unique_ptr<::etcd::Client> m_client;
        std::shared_ptr<::etcd::KeepAlive> m_keepalive;
    public:
        ServiceRegister(const std::string& EtcdAddrs);
        using ptr = std::shared_ptr<ServiceRegister>;
        using uptr = std::unique_ptr<ServiceRegister>;

        int Register(const std::string& ServicePath, const std::string& ServiceAddr,int TTL);
    private:
        bool CheckNotExist(const std::string& key);
    };

    //服务发现 example /XXX/XXX/ServiceName

    //服务发现
    class ServiceDiscovery
    {
    private:
        std::shared_ptr<::etcd::Client> m_client;
    public:
        using ptr = std::shared_ptr<ServiceDiscovery>;
        using uptr = std::shared_ptr<ServiceDiscovery>;
        ServiceDiscovery(const std::string& EtcdAddrs);
        //发现该前缀下所有节点
        EtcdAwaiter Discovery(const std::string& ServicePath);
    };


    //分布式锁
    class DistributedLock{
    private:
        std::unique_ptr<::etcd::Client> m_client;
        std::string m_lock_key;
    public:
        using ptr = std::shared_ptr<DistributedLock>;
        using uptr = std::unique_ptr<DistributedLock>;
        DistributedLock(const std::string& EtcdAddrs);

        void Lock(const std::string& key , int TTL);

        void UnLock();
    };
}


#endif