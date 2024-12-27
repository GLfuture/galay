#ifndef GALAY_EXTERNAPI_HPP
#define GALAY_EXTERNAPI_HPP


#include "Strategy.hpp"
#include "Scheduler.h"
#include "Coroutine.hpp"
#include "WaitAction.h"
#include "Time.h"
#include "Log.h"
#include <thread>
#include <concepts>
#include <openssl/ssl.h>


namespace galay 
{
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

using EeventSchedulerHolder = details::SchedulerHolder<details::RoundRobinLoadBalancer<details::EventScheduler>>;
using CoroutineSchedulerHolder = details::SchedulerHolder<details::RoundRobinLoadBalancer<details::CoroutineScheduler>>;
using TimerSchedulerHolder = details::SchedulerHolder<details::RoundRobinLoadBalancer<details::TimerScheduler>>;

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

//线程安全
template<typename CoRtn>
class WaitGroup
{
public:
    WaitGroup(uint32_t count);
    void Done();
    void Reset(uint32_t count = 0);
    AsyncResult<bool, CoRtn> Wait();
private:
    std::shared_mutex m_mutex;
    Coroutine<CoRtn>::wptr m_coroutine;
    uint32_t m_count = 0;
};


}


namespace galay::this_coroutine
{
/*
    父协程一定需要加入到协程存储
*/
//extern AsyncResult<void> AddToCoroutineStore();
template<typename CoRtn>
extern AsyncResult<typename CoroutineBase::wptr, CoRtn> GetThisCoroutine();

template<typename CoRtn>
extern AsyncResult<void, CoRtn> WaitAsyncExecute(const Coroutine<CoRtn>& co);

/*
    return false only when TimeScheduler is not running
    [ms] : ms
    [timer] : timer
    [scheduler] : coroutine_scheduler, this coroutine will resume at this scheduler
*/
template<typename CoRtn>
extern AsyncResult<void, CoRtn> Sleepfor(int64_t ms);

/*
    注意，直接传lambda会导致shared_ptr引用计数不增加，推荐使用bind,或者传lambda对象
    注意和AutoDestructor的回调区别，DeferExit的callback会在协程正常和非正常退出时调用
*/
template<typename CoRtn>
extern AsyncResult<void, CoRtn> DeferExit(const std::function<void(void)>& callback);

#define MIN_REMAIN_TIME     10
/*用在father 协程中，通过father销毁child*/
/*
    注意使用shared_ptr分配
    AutoDestructor的回调只会在超时时调用, 剩余时间小于MIN_REMAIN_TIME就会调用
*/
// class AutoDestructor: public std::enable_shared_from_this<AutoDestructor>
// {
// public:
//     using ptr = std::shared_ptr<AutoDestructor>;
//     AutoDestructor();
//     AsyncResult<void> Start(uint64_t ms);
//     void SetCallback(const std::function<void(void)>& callback) { this->m_callback = callback; }
//     bool Reflush();
//     ~AutoDestructor() = default;
// private:
//     std::shared_ptr<Timer> m_timer = nullptr;
//     std::atomic_uint64_t m_last_active_time = 0;
//     std::function<void(void)> m_callback;
// };
}

#include "ExternApi.tcc"

#endif