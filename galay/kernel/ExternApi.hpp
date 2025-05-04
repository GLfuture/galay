#ifndef GALAY_EXTERNAPI_HPP
#define GALAY_EXTERNAPI_HPP


#include "Strategy.hpp"
#include "Scheduler.h"
#include "Coroutine.hpp"
#include "WaitAction.h"
#include "Time.hpp"
#include "Log.h"
#include <thread>
#include <concepts>
#include <openssl/ssl.h>


namespace galay 
{

template <class F>
class DeferClass {
    public:
    DeferClass(F&& f) : m_func(std::forward<F>(f)) {}
    DeferClass(const F& f) : m_func(f) {}
    ~DeferClass() { m_func(); }

    DeferClass(const DeferClass& e) = delete;
    DeferClass& operator=(const DeferClass& e) = delete;

private:
    F m_func;
};

struct THost
{
    std::string m_ip;
    uint32_t m_port;
};

struct IOVec
{
    char* m_buffer = nullptr;           // buffer
    size_t m_size = 0;                  // buffer size
    size_t m_offset = 0;                // offset, recv和send都会改变值
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
    THost m_addr;
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

using EventSchedulerHolder = details::SchedulerHolder<details::RoundRobinLoadBalancer<details::EventScheduler>>;
using CoroutineSchedulerHolder = details::SchedulerHolder<details::RoundRobinLoadBalancer<details::CoroutineScheduler>>;

#ifdef __cplusplus
extern "C" {
#endif


bool InitializeSSLServerEnv(const char* cert_file, const char* key_file);
bool InitialiszeSSLClientEnv(const char* server_pem = nullptr);
bool DestroySSLEnv();

bool InitializeHttp2ServerEnv(const char* cert_file, const char* key_file);
bool InitializeHttp2ClientEnv(const char* server_pem = nullptr);
bool DestroyHttp2Env();

SSL_CTX* GetGlobalSSLCtx();

void InitializeGalayEnv(std::vector<std::unique_ptr<details::CoroutineScheduler>>&& coroutine_schedulers,
                        std::vector<std::unique_ptr<details::EventScheduler>>&& event_schedulers);
void StartGalayEnv();
void DestroyGalayEnv();



#ifdef __cplusplus
}
#endif

// To Erase
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
//extern AsyncResult<void> AddToCoroutineStore();
template<typename CoRtn>
extern AsyncResult<typename CoroutineBase::wptr, CoRtn> GetThisCoroutine();


/*
    return false only when TimeScheduler is not running
    [ms] : ms
    [timer] : timer
    [scheduler] : coroutine_scheduler, this coroutine will resume at this scheduler
*/
template<typename CoRtn>
extern AsyncResult<void, CoRtn> Sleepfor(int64_t ms);

template<typename CoRtn>
extern AsyncResult<void, CoRtn> DeferExit(const std::function<void(CoroutineBase::wptr)>& callback);


template<typename CoRtn, typename FCoRtn>
extern AsyncResult<typename Coroutine<CoRtn>::ptr, FCoRtn> WaitAsyncExecute(Coroutine<CoRtn>&& co);

template<typename CoRtn, typename FCoRtn\
    , std::enable_if_t<!std::is_void_v<CoRtn>, int> = 0>
extern AsyncResult<CoRtn, FCoRtn> WaitAsyncRtnExecute(Coroutine<CoRtn>&& co);

template<typename FCoRtn>
extern AsyncResult<void, FCoRtn> WaitAsyncRtnExecute(Coroutine<void>&& co);

/*
    协程内部同步接口
*/
template<typename Func, typename CoRtn, typename ...Args>
concept AsyncFuncType = requires(Func func, galay::RoutineCtx ctx, Args... args) {
    { std::forward<Func>(func)(ctx, std::forward<Args>(args)...) } 
    -> std::same_as<galay::Coroutine<CoRtn>>;
};

template<typename T>
concept RoutineCtxType = requires() {
    std::is_same_v<T, galay::RoutineCtx>;
};

template<typename CoRtn, typename FCoRtn, typename... Args, AsyncFuncType<CoRtn, Args...> Func\
    , RoutineCtxType FirstArg>
extern galay::AsyncResult<typename Coroutine<CoRtn>::ptr, FCoRtn> WaitAsyncExecute(Func func, FirstArg first, Args&&... args);

template<typename CoRtn, typename FCoRtn, typename... Args, AsyncFuncType<CoRtn, Args...> Func\
    , RoutineCtxType FirstArg, std::enable_if_t<!std::is_void_v<CoRtn>, int> = 0>
extern galay::AsyncResult<CoRtn, FCoRtn> WaitAsyncRtnExecute(Func func, FirstArg first, Args&&... args);

template<typename FCoRtn, typename... Args, AsyncFuncType<void, Args...> Func, RoutineCtxType FirstArg>
extern galay::AsyncResult<void, FCoRtn> WaitAsyncRtnExecute(Func func, FirstArg first, Args&&... args);

template<typename CoRtn, typename T>
concept CoroutineType = requires() {
    std::is_same_v<T, galay::Coroutine<CoRtn>>;
};

template<typename CoRtn,  typename FCoRtn, typename Class, CoroutineType<CoRtn> Ret\
    , RoutineCtxType FirstArg, typename ...FuncArgs, typename ...Args>
extern galay::AsyncResult<typename Coroutine<CoRtn>::ptr, FCoRtn> WaitAsyncExecute(Ret(Class::*func)(FirstArg, FuncArgs...), Class* obj, FirstArg first, Args&&... args);

template<typename CoRtn,  typename FCoRtn, typename Class, CoroutineType<CoRtn> Ret\
    , RoutineCtxType FirstArg, typename ...FuncArgs, typename ...Args, std::enable_if_t<!std::is_void_v<CoRtn>, int> = 0>
extern galay::AsyncResult<CoRtn, FCoRtn> WaitAsyncRtnExecute(Ret(Class::*func)(FirstArg, FuncArgs...), Class* obj, FirstArg first, Args&&... args);

template<typename FCoRtn, typename Class, CoroutineType<void> Ret, RoutineCtxType FirstArg
    , typename ...FuncArgs, typename ...Args>
extern galay::AsyncResult<void, FCoRtn> WaitAsyncRtnExecute(Ret(Class::*func)(FirstArg, FuncArgs...), Class* obj, FirstArg first, Args&&... args);



}

#include "ExternApi.tcc"

#endif