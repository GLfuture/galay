#include "AsyncEtcd.h"
#include <spdlog/spdlog.h>

galay::MiddleWare::AsyncEtcd::Etcd_Awaiter::Etcd_Awaiter(std::function<void(std::coroutine_handle<>,std::any&)>&& func)
{
    this->m_Func = func;
}

bool 
galay::MiddleWare::AsyncEtcd::Etcd_Awaiter::await_ready()
{
    return false;
}

void 
galay::MiddleWare::AsyncEtcd::Etcd_Awaiter::await_suspend(std::coroutine_handle<> handle)
{
    m_Func(handle,this->m_Result);
}

std::any 
galay::MiddleWare::AsyncEtcd::Etcd_Awaiter::await_resume()
{
    return m_Result;
}

galay::MiddleWare::AsyncEtcd::ServiceRegister::ServiceRegister(const std::string& EtcdAddrs)
{
    m_client = std::make_unique<etcd::Client>(EtcdAddrs);
}

int
galay::MiddleWare::AsyncEtcd::ServiceRegister::Register(const std::string& ServicePathAndNode, const std::string& ServiceAddr,int TTL)
{
    if(!CheckNotExist(ServicePathAndNode)) {
        spdlog::error("{} {} {} CheckNotExist error is Service is already exist",__TIME__,__FILE__,__LINE__);
        return -1;
    }
    m_keepalive = m_client->leasekeepalive(TTL).get();
    int64_t leaseid = m_keepalive->Lease();
    m_client->put(ServicePathAndNode,ServiceAddr,leaseid);
    return 0;
}

bool 
galay::MiddleWare::AsyncEtcd::ServiceRegister::CheckNotExist(const std::string &key)
{
    if (!m_client->get(key).get().value().as_string().empty())
        return false;
    return true;
}

galay::MiddleWare::AsyncEtcd::ServiceDiscovery::ServiceDiscovery(const std::string& EtcdAddrs)
{
    m_client = std::make_shared<etcd::Client>(EtcdAddrs);
}

galay::MiddleWare::AsyncEtcd::Etcd_Awaiter 
galay::MiddleWare::AsyncEtcd::ServiceDiscovery::Discovery(const std::string& ServicePath)
{
    std::weak_ptr client = m_client;
    return Etcd_Awaiter([client,ServicePath](std::coroutine_handle<> handle,std::any& res){
        client.lock()->ls(ServicePath).then([handle,&res](etcd::Response resp){
            auto keys = resp.keys();
            std::unordered_map<std::string,std::string> m;
            for(int i = 0 ; i < keys.size() ; i ++){
                auto node = keys[i].substr(keys[i].find_last_of('/')+1);
                m[node] = resp.values().at(i).as_string();
            }
            res = m;
            handle.resume();
        });
    });
}

