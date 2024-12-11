#include "ExternApi.h"
#include "Scheduler.h"
#include "Async.h"
#include "Event.h"
#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    #include <fcntl.h>
#endif

namespace galay
{

SSL_CTX* SslCtx = nullptr;
// API


bool InitializeSSLServerEnv(const char *cert_file, const char *key_file)
{
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    SslCtx = SSL_CTX_new(TLS_server_method());
    if (SslCtx == nullptr) {
        return false;
    }

    // 加载证书和私钥
    if (SSL_CTX_use_certificate_file(SslCtx, cert_file, SSL_FILETYPE_PEM) <= 0) {
        return false;
    }
    if (SSL_CTX_use_PrivateKey_file(SslCtx, key_file, SSL_FILETYPE_PEM) <= 0) {
        return false;
    }
    return true;
}

bool InitialiszeSSLClientEnv()
{
    SslCtx = SSL_CTX_new(TLS_client_method());
    if (SslCtx == nullptr) {
        return false;
    }
    return true;
}

bool DestroySSLEnv()
{
    SSL_CTX_free(SslCtx);
    EVP_cleanup();
    return true;
}

SSL_CTX *GetGlobalSSLCtx()
{
    return SslCtx;
}

void InitializeGalayEnv(std::pair<uint32_t, int> coroutineConf, std::pair<uint32_t, int> eventConf, std::pair<uint32_t, int> timerConf)
{
    CoroutineSchedulerHolder::GetInstance()->Initialize(coroutineConf.first, coroutineConf.second);
    EeventSchedulerHolder::GetInstance()->Initialize(eventConf.first, eventConf.second);
    TimerSchedulerHolder::GetInstance()->Initialize(timerConf.first, timerConf.second);
}

void DestroyGalayEnv()
{
    CoroutineSchedulerHolder::GetInstance()->Destroy();
    EeventSchedulerHolder::GetInstance()->Destroy();
    TimerSchedulerHolder::GetInstance()->Destroy();
}

}
