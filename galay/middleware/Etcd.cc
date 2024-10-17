#include "Etcd.h"
#include "../kernel/Scheduler.h"

namespace galay::middleware::etcd
{
EtcdResponse::EtcdResponse(::etcd::Response &response)
    : m_response(response)
{
}

bool EtcdResponse::Success()
{
    return m_response.is_ok();
}

std::string EtcdResponse::GetValue()
{
    return m_response.value().as_string();
}

std::vector<std::pair<std::string,std::string>> EtcdResponse::GetKeyValues()
{
    auto& keys = m_response.keys();
    auto& values = m_response.values();
    std::vector<std::pair<std::string,std::string>> result;
    for(int i = 0; i < keys.size(); ++i){
        result.push_back(std::make_pair(keys[i],values[i].as_string()));
    }
    return std::move(result);
}

EtcdClient::EtcdClient(const std::string& EtcdAddrs, int co_sche_index)
    : m_co_sche_index(co_sche_index)
{
    m_client = std::make_unique<::etcd::Client>(EtcdAddrs);
}

coroutine::Awaiter_bool 
EtcdClient::RegisterService(const std::string& ServiceName, const std::string& ServiceAddr)
{
    m_action.ResetCallback([this, ServiceName, ServiceAddr](coroutine::Coroutine* co) {
        this->m_client->put(ServiceName, ServiceAddr).then([this, co](::etcd::Response resp){
            m_response = std::move(resp);
            co->SetContext(true);
            scheduler::GetCoroutineScheduler(m_co_sche_index)->ResumeCoroutine(co);
        });
    });
    return coroutine::Awaiter_bool(&m_action);
}

coroutine::Awaiter_bool 
EtcdClient::RegisterService(const std::string& ServiceName, const std::string& ServiceAddr, int TTL)
{
    m_action.ResetCallback([this, ServiceName, ServiceAddr, TTL](coroutine::Coroutine* co){
        m_client->leasekeepalive(TTL).then([this,ServiceName,ServiceAddr,co](std::shared_ptr<::etcd::KeepAlive> keepalive){
            m_keepalive = keepalive;
            int64_t leaseid = keepalive->Lease();
            m_client->put(ServiceName,ServiceAddr,leaseid).then([this, co](::etcd::Response resp){
                m_response = std::move(resp);
                co->SetContext(true);
                scheduler::GetCoroutineScheduler(m_co_sche_index)->ResumeCoroutine(co);
            });
        });
    });
    return coroutine::Awaiter_bool(&m_action);
}

coroutine::Awaiter_bool 
EtcdClient::DiscoverService(const std::string& ServiceName)
{
    m_action.ResetCallback([this, ServiceName](coroutine::Coroutine* co){
        m_client->get(ServiceName).then([this, co](::etcd::Response resp){
            m_response = std::move(resp);
            co->SetContext(true);
            scheduler::GetCoroutineScheduler(m_co_sche_index)->ResumeCoroutine(co);
        });
    });
    return coroutine::Awaiter_bool(&m_action);
}

coroutine::Awaiter_bool 
EtcdClient::DiscoverServicePrefix(const std::string& Prefix)
{
    m_action.ResetCallback([this, Prefix](coroutine::Coroutine* co){
        m_client->ls(Prefix).then([this, co](::etcd::Response resp){
            m_response = std::move(resp);
            co->SetContext(true);
            scheduler::GetCoroutineScheduler(m_co_sche_index)->ResumeCoroutine(co);
        });
    });
    return coroutine::Awaiter_bool(&m_action);
}

EtcdResponse EtcdClient::GetResponse()
{
    return EtcdResponse(m_response);
}

void 
EtcdClient::Watch(const std::string& key, std::function<void(::etcd::Response)> handle)
{
    this->m_watcher = std::make_shared<::etcd::Watcher>(*(this->m_client), key, handle);
}

void 
EtcdClient::CancleWatch()
{
    if(!this->m_watcher->Cancelled()) this->m_watcher->Cancel();
}

void 
EtcdClient::Lock(const std::string& key)
{
    m_client->lock(key,true).get();
}

void 
EtcdClient::Lock(const std::string& key , int TTL)
{
    m_client->lock(key,TTL,true).get();
}

void 
EtcdClient::UnLock(const std::string& key)
{
    m_client->unlock(key).get();
}

bool 
EtcdClient::CheckExist(const std::string& key)
{
    if (!m_client->get(key).get().value().as_string().empty())
        return false;
    return true;
}

}
