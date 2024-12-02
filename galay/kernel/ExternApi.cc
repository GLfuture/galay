#include "ExternApi.h"
#include "Scheduler.h"
#include "Coroutine.h"
#include "Async.h"
#include "Event.h"
#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    #include <fcntl.h>
#endif

namespace galay
{

// API

IOVec VecDeepCopy(const IOVec *src)
{
    if(src == nullptr || src->m_length == 0 || src->m_buffer == nullptr || src->m_size == 0) {
        return IOVec{};
    }
    char* buffer = static_cast<char*>(malloc(src->m_length));
    memcpy(buffer, src->m_buffer, src->m_size);
    return IOVec {  .m_handle = src->m_handle,
                    .m_buffer= buffer, 
                    .m_size = src->m_size, 
                    .m_offset = src->m_offset,
                    .m_length = src->m_length};
}

bool VecMalloc(IOVec *src, size_t size)
{
    if (src == nullptr || src->m_size != 0 || src->m_buffer != nullptr) {
        return false;
    }
    src->m_buffer = static_cast<char*>(malloc(size));
    if (src->m_buffer == nullptr) {
        return false;
    }
    src->m_size = size;
    return true;
}

bool VecRealloc(IOVec *src, size_t size)
{
    if (src == nullptr || src->m_length == 0 || src->m_buffer == nullptr) {
        return false;
    }
    src->m_buffer = static_cast<char*>(realloc(src->m_buffer, size));
    if (src->m_buffer == nullptr) {
        return false;
    }
    src->m_size = size;
    return true;
}

bool VecFree(IOVec *src)
{
    if(src == nullptr || src->m_length == 0 || src->m_buffer == nullptr) {
        return false;
    }
    free(src->m_buffer);
    src->m_size = 0;
    src->m_length = 0;
    src->m_buffer = nullptr;
    src->m_offset = 0;
    return true;
}

bool AsyncSocket(async::AsyncNetIo *asocket)
{
    asocket->GetErrorCode() = error::ErrorCode::Error_NoError;
    asocket->GetHandle().fd = socket(AF_INET, SOCK_STREAM, 0);
    if (asocket->GetHandle().fd < 0) {
        asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_SocketError, errno);
        return false;
    }
    return true;
}

bool BindAndListen(async::AsyncNetIo *asocket, int port, int backlog)
{
    asocket->GetErrorCode() = error::ErrorCode::Error_NoError;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if( bind(asocket->GetHandle().fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) )
    {
        asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_BindError, errno);
        return false;
    }
    if( listen(asocket->GetHandle().fd, backlog) )
    {
        asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_ListenError, errno);
        return false;
    }
    return true;
}

coroutine::Awaiter_GHandle AsyncAccept(async::AsyncNetIo *asocket, NetAddr* addr)
{
    dynamic_cast<event::NetWaitEvent*>(asocket->GetAction()->GetBindEvent())->ResetNetWaitEventType(event::kTcpWaitEventTypeAccept);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {asocket->GetAction(), addr};
}

coroutine::Awaiter_bool AsyncConnect(async::AsyncNetIo *asocket, NetAddr* addr)
{
    dynamic_cast<event::NetWaitEvent*>(asocket->GetAction()->GetBindEvent())->ResetNetWaitEventType(event::kTcpWaitEventTypeConnect);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {asocket->GetAction(), addr};
}

coroutine::Awaiter_int AsyncRecv(async::AsyncNetIo *asocket, IOVec* iov)
{
    dynamic_cast<event::NetWaitEvent*>(asocket->GetAction()->GetBindEvent())->ResetNetWaitEventType(event::kTcpWaitEventTypeRecv);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {asocket->GetAction(), iov};
}

coroutine::Awaiter_int AsyncSend(async::AsyncNetIo *asocket, IOVec *iov)
{
    dynamic_cast<event::NetWaitEvent*>(asocket->GetAction()->GetBindEvent())->ResetNetWaitEventType(event::kTcpWaitEventTypeSend);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {asocket->GetAction(), iov};
}

coroutine::Awaiter_bool AsyncClose(async::AsyncNetIo *asocket)
{
    dynamic_cast<event::NetWaitEvent*>(asocket->GetAction()->GetBindEvent())->ResetNetWaitEventType(event::kTcpWaitEventTypeClose);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {asocket->GetAction(), nullptr};
}

bool AsyncSSLSocket(async::AsyncSslNetIo* asocket, SSL_CTX *ctx)
{
    SSL* ssl = SSL_new(ctx);
    if(ssl == nullptr) return false;
    if(SSL_set_fd(ssl, asocket->GetHandle().fd)) {
        asocket->GetSSL() = ssl;
        return true;
    }
    SSL_free(ssl);
    return false;
}

coroutine::Awaiter_bool AsyncSSLAccept(async::AsyncSslNetIo *asocket)
{
    dynamic_cast<event::NetWaitEvent*>(asocket->GetAction()->GetBindEvent())->ResetNetWaitEventType(event::kTcpWaitEventTypeSslAccept);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {asocket->GetAction(), nullptr};
}

coroutine::Awaiter_bool SSLConnect(async::AsyncSslNetIo *asocket)
{
    dynamic_cast<event::NetWaitEvent*>(asocket->GetAction()->GetBindEvent())->ResetNetWaitEventType(event::kTcpWaitEventTypeSslConnect);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {asocket->GetAction(), nullptr};
}

coroutine::Awaiter_int AsyncSSLRecv(async::AsyncSslNetIo *asocket, IOVec *iov)
{
    dynamic_cast<event::NetWaitEvent*>(asocket->GetAction()->GetBindEvent())->ResetNetWaitEventType(event::kTcpWaitEventTypeSslRecv);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {asocket->GetAction(), iov};
}

coroutine::Awaiter_int AsyncSSLSend(async::AsyncSslNetIo *asocket, IOVec *iov)
{
    dynamic_cast<event::NetWaitEvent*>(asocket->GetAction()->GetBindEvent())->ResetNetWaitEventType(event::kTcpWaitEventTypeSslSend);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {asocket->GetAction(), iov};
}

coroutine::Awaiter_bool AsyncSSLClose(async::AsyncSslNetIo *asocket)
{
    dynamic_cast<event::NetWaitEvent*>(asocket->GetAction()->GetBindEvent())->ResetNetWaitEventType(event::kTcpWaitEventTypeSslClose);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {asocket->GetAction(), nullptr};
}

coroutine::Awaiter_GHandle AsyncFileOpen(const char *path, const int flags, mode_t mode)
{
#if defined(__linux__) || defined(__APPLE__) 
    const int fd = open(path, flags);
#endif
    const auto handle = GHandle{fd};
    async::HandleOption option(handle);
    option.HandleNonBlock();
    return coroutine::Awaiter_GHandle(GHandle{fd});
}

coroutine::Awaiter_int AsyncFileRead(async::AsyncFileIo *afileio, IOVec *iov)
{
    dynamic_cast<event::FileIoWaitEvent*>(afileio->GetAction()->GetBindEvent())->ResetFileIoWaitEventType(event::kFileIoWaitEventTypeRead);
    afileio->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {afileio->GetAction(), iov};
}

coroutine::Awaiter_int AsyncFileWrite(async::AsyncFileIo *afileio, IOVec *iov)
{
    dynamic_cast<event::FileIoWaitEvent*>(afileio->GetAction()->GetBindEvent())->ResetFileIoWaitEventType(event::kFileIoWaitEventTypeWrite);
    afileio->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {afileio->GetAction(), iov};
}

//other

coroutine::CoroutineStore g_coroutine_store;

std::vector<scheduler::CoroutineScheduler*> g_coroutine_schedulers;
std::vector<scheduler::EventScheduler*> g_event_schedulers;
std::vector<scheduler::TimerScheduler*> g_timer_schedulers;
SSL_CTX* g_ssl_ctx = nullptr;

coroutine::CoroutineStore* GetCoroutineStore()
{
    return &g_coroutine_store;
}

bool InitializeSSLServerEnv(const char *cert_file, const char *key_file)
{
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    g_ssl_ctx = SSL_CTX_new(TLS_server_method());
    if (g_ssl_ctx == nullptr) {
        return false;
    }

    // 加载证书和私钥
    if (SSL_CTX_use_certificate_file(g_ssl_ctx, cert_file, SSL_FILETYPE_PEM) <= 0) {
        return false;
    }
    if (SSL_CTX_use_PrivateKey_file(g_ssl_ctx, key_file, SSL_FILETYPE_PEM) <= 0) {
        return false;
    }
    return true;
}

bool InitialiszeSSLClientEnv()
{
    g_ssl_ctx = SSL_CTX_new(TLS_client_method());
    if (g_ssl_ctx == nullptr) {
        return false;
    }
    return true;
}

bool DestroySSLEnv()
{
    SSL_CTX_free(g_ssl_ctx);
    EVP_cleanup();
    return true;
}

SSL_CTX *GetGlobalSSLCtx()
{
    return g_ssl_ctx;
}

void InitializeGalayEnv(int event_schedulers, int coroutine_schedulers, int timer_schedulers, int timeout)
{
    DynamicResizeCoroutineSchedulers(coroutine_schedulers);
    DynamicResizeEventSchedulers(event_schedulers);
    DynamicResizeTimerSchedulers(timer_schedulers);
    StartCoroutineSchedulers();
    StartEventSchedulers(timeout);
    StartTimerSchedulers(timeout);
}

void DestroyGalayEnv()
{
    StopCoroutineSchedulers();
    StopEventSchedulers();
    StopTimerSchedulers();
}

void DynamicResizeCoroutineSchedulers(const int num)
{
    const int now = static_cast<int>(g_coroutine_schedulers.size());
    if(const int sub = num - now; sub > 0) {
        for(int i = 0; i < sub; ++i) {
            g_coroutine_schedulers.push_back(new scheduler::CoroutineScheduler);
        }
    }else if(sub < 0) {
        for(int i = now - 1 ; i >= -sub ; -- i)
        {
            g_coroutine_schedulers[i]->Stop();
            delete g_coroutine_schedulers[i];
            g_coroutine_schedulers.erase(std::prev(g_coroutine_schedulers.end()));
        }
    }
}

void DynamicResizeEventSchedulers(const int num)
{
    const int now = static_cast<int>(g_event_schedulers.size());
    if(const int sub = num - now; sub > 0) {
        for(int i = 0; i < sub; ++i) {
            g_event_schedulers.push_back(new scheduler::EventScheduler);
        }
    } else if(sub < 0) {
        for(int i = now - 1 ; i >= -sub ; -- i)
        {
            g_event_schedulers[i]->Stop();
            delete g_event_schedulers[i];
            g_event_schedulers.erase(std::prev(g_event_schedulers.end()));
        }
    }
}

void DynamicResizeTimerSchedulers(const int num)
{
    const int now = static_cast<int>(g_timer_schedulers.size());
    if(const int sub = num - now; sub > 0) {
        for(int i = 0; i < sub; ++i) {
            g_timer_schedulers.push_back(new scheduler::TimerScheduler);
        }
    } else if(sub < 0) {
        for(int i = g_timer_schedulers.size() - 1 ; i >= -sub ; -- i)
        {
            g_timer_schedulers[i]->Stop();
            delete g_timer_schedulers[i];
            g_timer_schedulers.erase(std::prev(g_timer_schedulers.end()));
        }
    }
}

int GetCoroutineSchedulerNum()
{
    return g_coroutine_schedulers.size();
}

int GetEventSchedulerNum()
{
    return g_event_schedulers.size();
}

int GetTimerSchedulerNum()
{
    return g_timer_schedulers.size();
}

scheduler::EventScheduler* GetEventScheduler(int index)
{
    return g_event_schedulers[index];
}

static std::atomic_uint32_t g_current_coroutine_scheduler_index = 0;

scheduler::CoroutineScheduler *GetCoroutineSchedulerInOrder()
{
    const uint32_t size = g_coroutine_schedulers.size();
    uint32_t now = g_current_coroutine_scheduler_index.load();
    int retries = 0;
    while(retries < MAX_GET_COROUTINE_SCHEDULER_RETRY_TIMES){
        if( g_current_coroutine_scheduler_index.compare_exchange_strong(now, (now + 1) % size) ){
            return g_coroutine_schedulers[now];
        }
        std::this_thread::yield();
        now = g_current_coroutine_scheduler_index.load();
        ++retries;
    }
    return nullptr;
}

scheduler::CoroutineScheduler *GetCoroutineScheduler(int index)
{
    return g_coroutine_schedulers[index];
}

static std::atomic_uint32_t g_current_timer_scheduler_index = 0;

scheduler::TimerScheduler *GetTimerSchedulerInOrder()
{
    const uint32_t size = g_timer_schedulers.size();
    uint32_t now = g_current_timer_scheduler_index.load();
    int retries = 0;
    while(retries < MAX_GET_COROUTINE_SCHEDULER_RETRY_TIMES){
        if( g_current_timer_scheduler_index.compare_exchange_strong(now, (now + 1) % size) ){
            return g_timer_schedulers[now];
        }
        std::this_thread::yield();
        now = g_current_timer_scheduler_index.load();
        ++retries;
    }
    return nullptr;
}

scheduler::TimerScheduler *GetTimerScheduler(int index)
{
    return g_timer_schedulers[index];
}

void StartCoroutineSchedulers()
{
    for(const auto & g_coroutine_scheduler : g_coroutine_schedulers)
    {
        g_coroutine_scheduler->Loop();
    }
}

void StartEventSchedulers(int timeout)
{
    for(const auto & g_event_scheduler : g_event_schedulers)
    {
        g_event_scheduler->Loop(timeout);
    }
}

void StartTimerSchedulers(const int timeout)
{
    for(const auto & g_timer_scheduler : g_timer_schedulers)
    {
        g_timer_scheduler->Loop(timeout);
    }
}

void StopCoroutineSchedulers()
{
    for(const auto & g_coroutine_scheduler : g_coroutine_schedulers)
    {
        g_coroutine_scheduler->Stop();
        delete g_coroutine_scheduler;
    }
    g_coroutine_schedulers.clear();
}

void StopEventSchedulers()
{
    for(const auto & g_event_scheduler : g_event_schedulers)
    {
        g_event_scheduler->Stop();
        delete g_event_scheduler;
    }
    g_event_schedulers.clear();
}

void StopTimerSchedulers()
{
    for(int i = 0 ; i < g_timer_schedulers.size() ; ++i )
    {
        g_timer_schedulers[i]->Stop();
        delete g_timer_schedulers[i];
    }
    g_timer_schedulers.clear();
}

}
