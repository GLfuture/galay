#ifndef GALAY_EXTERNAPI_H
#define GALAY_EXTERNAPI_H

#include "Awaiter.h"
#include "Strategy.hpp"
#include "Log.h"
#include <thread>
#include <concepts>
#include <openssl/ssl.h>


namespace galay::details {
    class EventScheduler;
    class CoroutineScheduler;
    class TimerScheduler;
}

namespace galay::async {
    class AsyncNetIo;
    class AsyncSslNetIo;
    class AsyncFileIo;
}

namespace galay {
struct NetAddr
{
    std::string m_ip;
    uint16_t m_port;
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
    IOVecHolder(size_t size) { VecMalloc(&m_vec, size); }
    IOVecHolder(std::string&& buffer) {
        if(!buffer.empty()) {
            Reset(std::move(buffer));
        }
    }
    bool Realloc(size_t size)
    {
        return VecRealloc(&m_vec, size);
    }

    void ClearBuffer(){
        if(m_vec.m_buffer == nullptr) return;
        memset(m_vec.m_buffer, 0, m_vec.m_size);
        m_vec.m_offset = 0;
        m_vec.m_length = 0;
    }

    bool Reset(const IOVecType& iov)
    {
        if(iov.m_buffer == nullptr) return false;
        FreeMemory();
        m_vec.m_buffer = iov.m_buffer;
        m_vec.m_size = iov.m_size;
        m_vec.m_offset = iov.m_offset;
        m_vec.m_length = iov.m_length;
        return true;
    }

    bool Reset()
    {
        return FreeMemory();
    }

    bool Reset(std::string&& str)
    {
        FreeMemory();
        m_temp = std::forward<std::string>(str);
        m_vec.m_buffer = m_temp.data();
        m_vec.m_size = m_temp.length();
        return true;
    }

    IOVecType* operator->()
    {
        return &m_vec;
    }

    IOVecType* operator &()
    {
        return &m_vec;
    }

    ~IOVecHolder()
    {
        FreeMemory();    
    }
private:

    bool VecMalloc(IOVecType* src, size_t size)
    {
        if (src == nullptr || src->m_size != 0 || src->m_buffer != nullptr) {
            return false;
        }
        src->m_buffer = static_cast<char*>(calloc(size, sizeof(char)));
        if (src->m_buffer == nullptr) {
            return false;
        }
        src->m_size = size;
        return true;
    }
    
    bool VecRealloc(IOVecType* src, size_t size)
    {
        if (src == nullptr || src->m_buffer == nullptr) {
            return false;
        }
        src->m_buffer = static_cast<char*>(realloc(src->m_buffer, size));
        if (src->m_buffer == nullptr) {
            return false;
        }
        src->m_size = size;
        return true;
    }

    bool VecFree(IOVecType* src)
    {
        if(src == nullptr || src->m_size == 0 || src->m_buffer == nullptr) {
            return false;
        }
        free(src->m_buffer);
        src->m_size = 0;
        src->m_length = 0;
        src->m_buffer = nullptr;
        src->m_offset = 0;
        return true;
    }

    bool FreeMemory() 
    {
        bool res;
        if(m_temp.empty()) {
            return VecFree(&m_vec);
        } else {
            if(m_temp.data() != m_vec.m_buffer) {
                res = VecFree(&m_vec);
            }
        }
        m_temp.clear();
        return true;
    }
private:
    IOVecType m_vec;
    std::string m_temp;
};

template<>
inline bool IOVecHolder<UdpIOVec>::Reset(const UdpIOVec& iov)
{
    if(iov.m_buffer == nullptr) return false;
    FreeMemory();
    m_vec.m_buffer = iov.m_buffer;
    m_vec.m_size = iov.m_size;
    m_vec.m_offset = iov.m_offset;
    m_vec.m_length = iov.m_length;
    m_vec.m_addr = iov.m_addr;
    return true;
}



#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif


/*
    ****************************
                net
    ****************************
*/

bool AsyncTcpSocket(async::AsyncNetIo* asocket);
bool AsyncUdpSocket(async::AsyncNetIo* asocket);
bool BindAndListen(async::AsyncNetIo* asocket, int port, int backlog);

coroutine::Awaiter_GHandle AsyncAccept(async::AsyncNetIo* asocket, NetAddr* addr);
coroutine::Awaiter_bool AsyncConnect(async::AsyncNetIo* async_socket, NetAddr* addr);
/*
    return: 
        >0   bytes read
        0   close connection
        <0  error
*/
coroutine::Awaiter_int AsyncRecv(async::AsyncNetIo* asocket, TcpIOVec* iov, size_t length);
/*
    return: 
        >0   bytes send
        <0  error
*/
coroutine::Awaiter_int AsyncSend(async::AsyncNetIo* asocket, TcpIOVec* iov, size_t length);
/*

*/
coroutine::Awaiter_int AsyncRecvFrom(async::AsyncNetIo* asocket, UdpIOVec* iov, size_t length);
coroutine::Awaiter_int AsyncSendTo(async::AsyncNetIo* asocket, UdpIOVec* iov, size_t length);
coroutine::Awaiter_bool AsyncClose(async::AsyncNetIo* asocket);

bool AsyncSSLSocket(async::AsyncSslNetIo* asocket, SSL_CTX* ctx);
/*
    must be called after AsyncAccept
*/
coroutine::Awaiter_bool AsyncSSLAccept(async::AsyncSslNetIo* asocket);
/*
    must be called after AsyncConnect
*/
coroutine::Awaiter_bool SSLConnect(async::AsyncSslNetIo* asocket);
coroutine::Awaiter_int AsyncSSLRecv(async::AsyncSslNetIo* asocket, TcpIOVec *iov, size_t length);
coroutine::Awaiter_int AsyncSSLSend(async::AsyncSslNetIo* asocket, TcpIOVec *iov, size_t length);
coroutine::Awaiter_bool AsyncSSLClose(async::AsyncSslNetIo* asocket);

/*
    ****************************
                file 
    ****************************
*/
coroutine::Awaiter_GHandle AsyncFileOpen(const char* path, int flags, mode_t mode);
coroutine::Awaiter_int AsyncFileRead(async::AsyncFileIo* afile, FileIOVec* iov, size_t length);
coroutine::Awaiter_int AsyncFileWrite(async::AsyncFileIo* afile, FileIOVec* iov, size_t length);


/*
   OpenSSL 
*/

bool InitializeSSLServerEnv(const char* cert_file, const char* key_file);
bool InitialiszeSSLClientEnv();
bool DestroySSLEnv();
SSL_CTX* GetGlobalSSLCtx();


template<typename T>
concept LoadBalancerType = requires(T t)
{
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
void Initialize(uint32_t scheduler_size, int timeout)
{
    if(scheduler_size == 0) scheduler_size = 1;
    std::vector<typename Type::value_type*> scheduler_ptrs;
    for (int i = 0; i < scheduler_size; ++i)
    {
        int try_count = MAX_START_SCHEDULERS_RETRY;
        m_schedulers.emplace_back(std::make_unique<typename Type::value_type>());
        scheduler_ptrs.emplace_back(m_schedulers[i].get());
        m_schedulers[i]->Loop(timeout);
        while(!m_schedulers[i]->IsRunning() && try_count-- >= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        if(try_count <= 0) {
            LogTrace("Start coroutine scheduler failed, index: {}", i);
            exit(-1);
        }
    }
    SchedulerLoadBalancer = std::make_unique<Type>(scheduler_ptrs);
}

void Initialize(std::vector<std::unique_ptr<typename Type::value_type>>&& schedulers)
{
    m_schedulers = std::forward<decltype(schedulers)>(schedulers);
    std::vector<typename Type::value_type*> scheduler_ptrs;
    scheduler_ptrs.reserve(m_schedulers.size());
    for(auto& scheduler : m_schedulers) {
        scheduler_ptrs.push_back(scheduler.get());
    }
    SchedulerLoadBalancer = std::make_unique<Type>(scheduler_ptrs);
}

void Destroy()
{
    for( auto& scheduler : m_schedulers) {
        scheduler->Stop();
    }
}

static SchedulerHolder* GetInstance()
{
    if(!Instance) {
        Instance = std::make_unique<SchedulerHolder>();
    }
    return Instance.get();
}

Type::value_type* GetScheduler()
{
    return SchedulerLoadBalancer.get()->Select();
}

Type::value_type* GetScheduler(uint32_t index)
{
    return m_schedulers[index].get();
}

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

extern "C" {

void InitializeGalayEnv(std::pair<uint32_t, int> coroutineConf, std::pair<uint32_t, int> eventConf, std::pair<uint32_t, int> timerConf);
void DestroyGalayEnv();

}

}

#endif