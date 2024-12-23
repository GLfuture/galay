#ifndef GALAY_EXTERNAPI_HPP
#define GALAY_EXTERNAPI_HPP


#include "Strategy.hpp"
#include "Scheduler.h"
#include "Log.h"
#include <thread>
#include <concepts>
#include <openssl/ssl.h>


namespace galay {
struct NetAddr
{
    std::string m_ip;
    uint32_t m_port;
};

struct IOVec
{
    char* m_buffer = nullptr;           // buffer
    size_t m_size = 0;                  // buffer size
    size_t m_offset = 0;                // offset
    size_t m_length = 0;                // operation length
};

struct TcpIOVec: public IOVec
{
};

struct FileIOVec: public IOVec
{
};


struct UdpIOVec: public IOVec
{
    NetAddr m_addr;
};

template<typename T>
concept VecBase = requires(T t) {
    std::is_base_of_v<IOVec, T>;
};

template <VecBase IOVecType>
class  IOVecHolder
{
public:
    IOVecHolder() = default;
    IOVecHolder(size_t size);
    IOVecHolder(std::string&& buffer);
    bool Realloc(size_t size);
    void ClearBuffer();
    bool Reset(const IOVecType& iov);
    bool Reset();
    bool Reset(std::string&& str);
    IOVecType* operator->();
    IOVecType* operator &();
    ~IOVecHolder();
private:
    bool VecMalloc(IOVecType* src, size_t size);
    bool VecRealloc(IOVecType* src, size_t size);
    bool VecFree(IOVecType* src);
    bool FreeMemory();
private:
    IOVecType m_vec;
    std::string m_temp;
};


template<typename T>
concept LoadBalancerType = requires(T t)
{
    typename T::value_type;
    requires std::is_constructible_v<T, const std::vector<typename T::value_type*>&>;
    requires std::is_same_v<decltype(t.Select()), typename T::value_type*>;
};

#define MAX_START_SCHEDULERS_RETRY   50

template<LoadBalancerType Type>
class SchedulerHolder
{
    static std::unique_ptr<Type> SchedulerLoadBalancer;
    static std::unique_ptr<SchedulerHolder> Instance;
public:

    //最少为1
    void Initialize(uint32_t scheduler_size, int timeout);
    void Initialize(std::vector<std::unique_ptr<typename Type::value_type>>&& schedulers);
    void Destroy();
    static SchedulerHolder* GetInstance();
    Type::value_type* GetScheduler();
    Type::value_type* GetScheduler(uint32_t index);
private:
    std::vector<std::unique_ptr<typename Type::value_type>> m_schedulers;
};

template<LoadBalancerType Type>
std::unique_ptr<Type> SchedulerHolder<Type>::SchedulerLoadBalancer;

template<LoadBalancerType Type>
std::unique_ptr<SchedulerHolder<Type>> SchedulerHolder<Type>::Instance;

using EeventSchedulerHolder = SchedulerHolder<details::RoundRobinLoadBalancer<details::EventScheduler>>;
using CoroutineSchedulerHolder = SchedulerHolder<details::RoundRobinLoadBalancer<details::CoroutineScheduler>>;
using TimerSchedulerHolder = SchedulerHolder<details::RoundRobinLoadBalancer<details::TimerScheduler>>;

#ifdef __cplusplus
extern "C" {
#endif


bool InitializeSSLServerEnv(const char* cert_file, const char* key_file);
bool InitialiszeSSLClientEnv();
bool DestroySSLEnv();
SSL_CTX* GetGlobalSSLCtx();
void InitializeGalayEnv(std::pair<uint32_t, int> coroutineConf, std::pair<uint32_t, int> eventConf, std::pair<uint32_t, int> timerConf);
void DestroyGalayEnv();


#ifdef __cplusplus
}
#endif
}

#include "ExternApi.tcc"

#endif