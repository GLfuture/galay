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
extern AsyncResult<CoRtn, CoRtn> WaitAsyncExecute(Coroutine<CoRtn>&& co);

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

/*
    协程内部同步接口
*/
template<typename T, typename ...Args>
concept AsyncFuncType = requires(T func, galay::RoutineCtx ctx, Args... args) {
    func(ctx, args...); // 要求 T 类型的对象可以调用，并且第一个参数必须是 RoutineCtx 类型
};

template<AsyncFuncType Func, typename ...Args>
extern galay::AsyncResult<void, void> WaitAsyncExecute(Func func, Args&&... args);

template<typename T>
concept RoutineCtxType = requires() {
    std::is_same_v<T, galay::RoutineCtx> ;
};

template<typename Class, typename Ret, RoutineCtxType FirstArg, typename ...FuncArgs, typename ...Args>
extern galay::AsyncResult<void, void> WaitAsyncExecute(Ret(Class::*func)(FirstArg, FuncArgs...), Class* obj, Args&&... args);

}

#include "ExternApi.tcc"

#endif