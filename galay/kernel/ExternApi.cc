#include "ExternApi.h"
#include "Scheduler.h"

namespace galay
{

std::vector<scheduler::CoroutineScheduler*> g_coroutine_schedulers;
std::vector<scheduler::EventScheduler*> g_netio_schedulers;

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
    int now = g_netio_schedulers.size();
    int sub = num - now;
    if(sub > 0) {
        for(int i = 0; i < sub; ++i) {
            g_netio_schedulers.push_back(new scheduler::EventScheduler);
        }
    } else if(sub < 0) {
        for(int i = g_netio_schedulers.size() - 1 ; i >= -sub ; -- i)
        {
            g_netio_schedulers[i]->Stop();
            delete g_netio_schedulers[i];
            g_netio_schedulers.erase(std::prev(g_netio_schedulers.end()));
        }
    }
}

int GetCoroutineSchedulerNum()
{
    return g_coroutine_schedulers.size();
}

int GetEventSchedulerNum()
{
    return g_netio_schedulers.size();
}

scheduler::EventScheduler* GetEventScheduler(int index)
{
    return g_netio_schedulers[index];
}

scheduler::CoroutineScheduler* GetCoroutineScheduler(int index)
{
    return g_coroutine_schedulers[index];
}

void StartAllCoroutineSchedulers()
{
    for(int i = 0 ; i < g_coroutine_schedulers.size() ; ++i )
    {
        g_coroutine_schedulers[i]->Loop();
    }
}

void StartAllEventSchedulers(int timeout)
{
    for(int i = 0 ; i < g_netio_schedulers.size() ; ++i )
    {
        g_netio_schedulers[i]->Loop(timeout);
    }
}

void StopAllCoroutineSchedulers()
{
    for(int i = 0 ; i < g_coroutine_schedulers.size() ; ++i )
    {
        g_coroutine_schedulers[i]->Stop();
        delete g_coroutine_schedulers[i];
    }
    g_coroutine_schedulers.clear();
}

void StopAllEventSchedulers()
{
    for(int i = 0 ; i < g_netio_schedulers.size() ; ++i )
    {
        g_netio_schedulers[i]->Stop();
        delete g_netio_schedulers[i];
    }
    g_netio_schedulers.clear();
}


SSL_CTX* g_ssl_ctx = nullptr;

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


}
