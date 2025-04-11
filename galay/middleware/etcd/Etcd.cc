#include "Etcd.hpp"
#include "galay/kernel/Scheduler.h"
#include "galay/kernel/ExternApi.hpp"

bool galay::details::EtcdAction::DoAction(CoroutineBase::wptr co, void *ctx)
{
    m_callback(co, ctx);
    return true;
}



namespace galay::etcd
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

EtcdClient::EtcdClient(const std::string& EtcdAddrs)
{
    m_client = std::make_unique<::etcd::Client>(EtcdAddrs);
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