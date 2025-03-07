#ifndef GALAY_ETCD_TCC
#define GALAY_ETCD_TCC

#include "Etcd.hpp"

namespace galay::etcd
{

template<typename CoRtn>
AsyncResult<bool, CoRtn> 
EtcdClient::RegisterService(const std::string& ServiceName, const std::string& ServiceAddr)
{
    m_action.ResetCallback([this, ServiceName, ServiceAddr](CoroutineBase::wptr co, void* ctx) {
        this->m_client->put(ServiceName, ServiceAddr).then([this, co](::etcd::Response resp){
            m_response = std::move(resp);
            static_cast<details::Awaiter<bool>*>(co.lock()->GetAwaiter())->SetResult(true);
            co.lock()->GetCoScheduler()->ToResumeCoroutine(co);
        });
    });
    return {&m_action, nullptr};
}

template<typename CoRtn>
AsyncResult<bool,CoRtn>
EtcdClient::RegisterService(const std::string& ServiceName, const std::string& ServiceAddr, int TTL)
{
    m_action.ResetCallback([this, ServiceName, ServiceAddr, TTL](CoroutineBase::wptr co, void* ctx) {
        m_client->leasekeepalive(TTL).then([this,ServiceName,ServiceAddr,co](std::shared_ptr<::etcd::KeepAlive> keepalive){
            m_keepalive = keepalive;
            int64_t leaseid = keepalive->Lease();
            m_client->put(ServiceName,ServiceAddr,leaseid).then([this, co](::etcd::Response resp){
                m_response = std::move(resp);
                static_cast<details::Awaiter<bool>*>(co.lock()->GetAwaiter())->SetResult(true);
                co.lock()->GetCoScheduler()->ToResumeCoroutine(co);
            });
        });
    });
    return {&m_action, nullptr};
}

template<typename CoRtn>
AsyncResult<bool, CoRtn> 
EtcdClient::DiscoverService(const std::string& ServiceName)
{
    m_action.ResetCallback([this, ServiceName](CoroutineBase::wptr co, void* ctx){
        m_client->get(ServiceName).then([this, co](::etcd::Response resp){
            m_response = std::move(resp);
            static_cast<details::Awaiter<bool>*>(co.lock()->GetAwaiter())->SetResult(true);
            co.lock()->GetCoScheduler()->ToResumeCoroutine(co);
        });
    });
    return {&m_action, nullptr};
}

template<typename CoRtn>
AsyncResult<bool, CoRtn> 
EtcdClient::DiscoverServicePrefix(const std::string& Prefix)
{
    m_action.ResetCallback([this, Prefix](CoroutineBase::wptr co, void* ctx){
        m_client->ls(Prefix).then([this, co](::etcd::Response resp){
            m_response = std::move(resp);
            static_cast<details::Awaiter<bool>*>(co.lock()->GetAwaiter())->SetResult(true);
            co.lock()->GetCoScheduler()->ToResumeCoroutine(co);
        });
    });
    return {&m_action, nullptr};
}


}

#endif