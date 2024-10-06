#include "etcd.h"
#include <spdlog/spdlog.h>

namespace galay::middleware::etcd
{

std::unique_ptr<ServiceCenter> ServiceCenter::m_instance;

ServiceCenter* 
ServiceCenter::GetInstance()
{
    if(!m_instance)
    {
        m_instance = std::make_unique<ServiceCenter>();
    }
    return m_instance.get();
}

std::string 
ServiceCenter::GetServiceAddr(const std::string &ServiceName)
{
    std::shared_lock lock(m_mutex);
    auto it = m_serviceAddr.Find(ServiceName);
    if(it != m_serviceAddr.End()){
        return it->second;
    }
    return "";
}

void 
ServiceCenter::SetServiceAddr(const std::string &ServiceName, const std::string &ServiceAddr)
{
    std::unique_lock lock(m_mutex);
    m_serviceAddr[ServiceName] = ServiceAddr;
}



std::shared_mutex m_mutex;
std::unordered_map<std::string, std::string> m_serviceAddr;

EtcdResult::EtcdResult(result::ResultInterface::ptr result)
{
    this->m_result = result;
}

//DiscoverService
std::string 
EtcdResult::ServiceAddr()
{
    return std::any_cast<std::string>(m_result->Result());
}

//DiscoverServicePrefix
std::vector<std::pair<std::string, std::string>> 
EtcdResult::ServiceAddrs()
{
    return std::any_cast<std::vector<std::pair<std::string, std::string>>>(m_result->Result());
}

bool 
EtcdResult::Success()
{
    return m_result->Success();
}

std::string 
EtcdResult::Error()
{
    return m_result->Error();
}

coroutine::GroupAwaiter& 
EtcdResult::Wait()
{
    return m_result->Wait();
}

EtcdClient::EtcdClient(const std::string& EtcdAddrs)
{
    m_client = std::make_unique<::etcd::Client>(EtcdAddrs);
}

EtcdResult 
EtcdClient::RegisterService(const std::string& ServiceName, const std::string& ServiceAddr)
{
    result::ResultInterface::ptr result = std::make_shared<result::ResultInterface>();
    result->AddTaskNum(1);
    this->m_client->put(ServiceName, ServiceAddr).then([result](::etcd::Response resp){
        if(resp.is_ok())
        {
            result->SetSuccess(true);
        }
        else
        {
            result->SetErrorMsg(resp.error_message());
        }
        result->Done();
    });

    return EtcdResult(result);
}

EtcdResult 
EtcdClient::RegisterService(const std::string& ServiceName, const std::string& ServiceAddr, int TTL)
{
    result::ResultInterface::ptr result = std::make_shared<result::ResultInterface>();
    result->AddTaskNum(1);
    this->m_client->leasekeepalive(TTL).then([result,this,ServiceName,ServiceAddr](std::shared_ptr<::etcd::KeepAlive> keepalive){
        this->m_keepalive = keepalive;
        int64_t leaseid = keepalive->Lease();
        m_client->put(ServiceName,ServiceAddr,leaseid).then([result](::etcd::Response resp){
            if(resp.is_ok())
            {
                result->SetSuccess(true);
            }
            else
            {
                result->SetErrorMsg(resp.error_message());
            }
            result->Done();
        });
    });

    return EtcdResult(result);
}

EtcdResult 
EtcdClient::DiscoverService(const std::string& ServiceName)
{
    result::ResultInterface::ptr result = std::make_shared<result::ResultInterface>();
    result->AddTaskNum(1);
    m_client->get(ServiceName).then([result](::etcd::Response resp){
        if(!resp.is_ok())
        {
            result->SetErrorMsg(resp.error_message());
            result->Done();
        }
        else{
            result->SetResult(resp.value().as_string());
            result->SetSuccess(true);
            result->Done();
        }
    });
    return EtcdResult(result);
}

EtcdResult 
EtcdClient::DiscoverServicePrefix(const std::string& Prefix)
{
    result::ResultInterface::ptr result = std::make_shared<result::ResultInterface>();
    result->AddTaskNum(1);
    m_client->ls(Prefix).then([result,this,Prefix](::etcd::Response resp){
        if(!resp.is_ok()){
            result->SetErrorMsg(resp.error_message());
            result->Done();
        }
        else{
            auto keys = resp.keys();
            std::vector<std::pair<std::string, std::string>> temp(keys.size());
            for(int i = 0 ; i < keys.size() ; i ++){
                temp[i] = { keys[i], resp.values().at(i).as_string()};
            }
            result->SetResult(temp);
            result->SetSuccess(true);
            result->Done();
        }
    });
    return EtcdResult(result);
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
    this->m_lockKey = key;
    m_client->lock(key,true).get();
}

void 
EtcdClient::Lock(const std::string& key , int TTL)
{
    this->m_lockKey = key;
    m_client->lock(key,TTL,true).get();
}

void 
EtcdClient::UnLock()
{
    m_client->unlock(this->m_lockKey).get();
}

bool 
EtcdClient::CheckExist(const std::string& key)
{
    if (!m_client->get(key).get().value().as_string().empty())
        return false;
    return true;
}
}