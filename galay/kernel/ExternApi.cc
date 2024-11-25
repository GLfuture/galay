#include "ExternApi.h"
#include "Scheduler.h"
#include "Coroutine.h"

namespace galay
{
thread_local coroutine::CoroutineStore g_coroutine_store;

std::vector<scheduler::CoroutineScheduler*> g_coroutine_schedulers;
std::vector<scheduler::EventScheduler*> g_event_schedulers;
std::vector<scheduler::TimerScheduler*> g_timer_schedulers;
SSL_CTX* g_ssl_ctx = nullptr;


coroutine::CoroutineStore* GetThisThreadCoroutineStore()
{
    return &g_coroutine_store;
}

bool InitialSSLServerEnv(const char *cert_file, const char *key_file)
{
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    g_ssl_ctx = SSL_CTX_new(TLS_server_method());
    if (g_ssl_ctx == NULL) {
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

bool InitialSSLClientEnv()
{
    g_ssl_ctx = SSL_CTX_new(TLS_client_method());
    if (g_ssl_ctx == NULL) {
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


void DynamicResizeCoroutineSchedulers(int num)
{
    int now = g_coroutine_schedulers.size();
    int sub = num - now;
    if(sub > 0) {
        for(int i = 0; i < sub; ++i) {
            g_coroutine_schedulers.push_back(new scheduler::CoroutineScheduler);
        }
    }else if(sub < 0) {
        for(int i = g_coroutine_schedulers.size() - 1 ; i >= -sub ; -- i)
        {
            g_coroutine_schedulers[i]->Stop();
            delete g_coroutine_schedulers[i];
            g_coroutine_schedulers.erase(std::prev(g_coroutine_schedulers.end()));
        }
    }
}

void DynamicResizeEventSchedulers(int num)
{
    int now = g_event_schedulers.size();
    int sub = num - now;
    if(sub > 0) {
        for(int i = 0; i < sub; ++i) {
            g_event_schedulers.push_back(new scheduler::EventScheduler);
        }
    } else if(sub < 0) {
        for(int i = g_event_schedulers.size() - 1 ; i >= -sub ; -- i)
        {
            g_event_schedulers[i]->Stop();
            delete g_event_schedulers[i];
            g_event_schedulers.erase(std::prev(g_event_schedulers.end()));
        }
    }
}

void DynamicResizeTimerSchedulers(int num)
{
    int now = g_timer_schedulers.size();
    int sub = num - now;
    if(sub > 0) {
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
    uint32_t size = g_coroutine_schedulers.size();
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
    uint32_t size = g_timer_schedulers.size();
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

void StartAllSchedulers(int timeout)
{
    StartCoroutineSchedulers();
    StartEventSchedulers(timeout);
    StartTimerSchedulers(timeout);
}

void StartCoroutineSchedulers()
{
    for(int i = 0 ; i < g_coroutine_schedulers.size() ; ++i )
    {
        g_coroutine_schedulers[i]->Loop();
    }
}

void StartEventSchedulers(int timeout)
{
    for(int i = 0 ; i < g_event_schedulers.size() ; ++i )
    {
        g_event_schedulers[i]->Loop(timeout);
    }
}

void StartTimerSchedulers(int timeout)
{
    for(int i = 0 ; i < g_timer_schedulers.size() ; ++i )
    {
        g_timer_schedulers[i]->Loop(timeout);
    }
}

void StopAllSchedulers()
{
    StopCoroutineSchedulers();
    StopEventSchedulers();
    StopTimerSchedulers();
}

void StopCoroutineSchedulers()
{
    for(int i = 0 ; i < g_coroutine_schedulers.size() ; ++i )
    {
        g_coroutine_schedulers[i]->Stop();
        delete g_coroutine_schedulers[i];
    }
    g_coroutine_schedulers.clear();
}

void StopEventSchedulers()
{
    for(int i = 0 ; i < g_event_schedulers.size() ; ++i )
    {
        g_event_schedulers[i]->Stop();
        delete g_event_schedulers[i];
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
