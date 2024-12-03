#ifndef GALAY_EXTERNAPI_H
#define GALAY_EXTERNAPI_H

#include "Awaiter.h"
#include <openssl/ssl.h>


namespace galay::scheduler {
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

struct IOVec
{
    GHandle m_handle{};
    char* m_buffer = nullptr;           // buffer
    size_t m_size = 0;                  // buffer size
    size_t m_offset = 0;                // offset
    size_t m_length = 0;                // operation length
};

class IOVecHolder
{
public:
    IOVecHolder(size_t size);
    IOVecHolder();
    void Realloc(size_t size);
    bool Reset(const IOVec& iov);
    bool Reset();
    bool Reset(std::string&& str);
    IOVec* operator->();
    IOVec* operator &();
    ~IOVecHolder();
private:
    bool FreeMemory();
private:
    IOVec m_vec;
    std::string m_temp;
};

struct NetAddr
{
    std::string m_ip;
    uint16_t m_port;
};


extern "C" {

/*
    ****************************
            Vec Help
    ****************************
*/
void VecFull(IOVec* src, char* buffer, size_t size, size_t offset, size_t length);
IOVec VecDeepCopy(const IOVec* src);
bool VecMalloc(IOVec* src, size_t size);
bool VecRealloc(IOVec* src, size_t size);
bool VecFree(IOVec* src);


/*
    ****************************
                net
    ****************************
*/

bool AsyncSocket(async::AsyncNetIo* asocket);
bool BindAndListen(async::AsyncNetIo* asocket, int port, int backlog);

coroutine::Awaiter_GHandle AsyncAccept(async::AsyncNetIo* asocket, NetAddr* addr);
coroutine::Awaiter_bool AsyncConnect(async::AsyncNetIo* async_socket, NetAddr* addr);
/*
    return: 
        >0   bytes read
        0   close connection
        <0  error
*/
coroutine::Awaiter_int AsyncRecv(async::AsyncNetIo* asocket, IOVec* iov, size_t length);
/*
    return: 
        >0   bytes send
        <0  error
*/
coroutine::Awaiter_int AsyncSend(async::AsyncNetIo* asocket, IOVec* iov, size_t length);
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
coroutine::Awaiter_int AsyncSSLRecv(async::AsyncSslNetIo* asocket, IOVec *iov, size_t length);
coroutine::Awaiter_int AsyncSSLSend(async::AsyncSslNetIo* asocket, IOVec *iov, size_t length);
coroutine::Awaiter_bool AsyncSSLClose(async::AsyncSslNetIo* asocket);

/*
    ****************************
                file 
    ****************************
*/
coroutine::Awaiter_GHandle AsyncFileOpen(const char* path, int flags, mode_t mode);
coroutine::Awaiter_int AsyncFileRead(async::AsyncFileIo* afile, IOVec* iov, size_t length);
coroutine::Awaiter_int AsyncFileWrite(async::AsyncFileIo* afile, IOVec* iov, size_t length);


#define MAX_GET_COROUTINE_SCHEDULER_RETRY_TIMES     50

/*
    Coroutine
*/
coroutine::CoroutineStore* GetCoroutineStore();

/*
   OpenSSL 
*/

bool InitializeSSLServerEnv(const char* cert_file, const char* key_file);
bool InitialiszeSSLClientEnv();
bool DestroySSLEnv();
SSL_CTX* GetGlobalSSLCtx();


/*
    Scheduler
*/

void InitializeGalayEnv(int event_schedulers, int coroutine_schedulers, int timer_schedulers, int timeout);
void DestroyGalayEnv();


// details
void DynamicResizeCoroutineSchedulers(int num);
void DynamicResizeEventSchedulers(int num);
void DynamicResizeTimerSchedulers(int num);

int GetCoroutineSchedulerNum();
int GetEventSchedulerNum();
int GetTimerSchedulerNum();

scheduler::EventScheduler* GetEventScheduler(int index);
scheduler::CoroutineScheduler* GetCoroutineSchedulerInOrder();
scheduler::CoroutineScheduler* GetCoroutineScheduler(int index);
scheduler::TimerScheduler* GetTimerSchedulerInOrder();
scheduler::TimerScheduler* GetTimerScheduler(int index);

/*
    Start all schedulers
    [timeout]: timeout in milliseconds, -1 will block
*/
void StartCoroutineSchedulers();
void StartEventSchedulers(int timeout);
void StartTimerSchedulers(int timeout);
/*
    Stop all schedulers
*/
void StopCoroutineSchedulers();
void StopEventSchedulers(); 
void StopTimerSchedulers();

}


}

#endif