#include "ExternApi.hpp"
#include "Scheduler.h"
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

void InitializeGalayEnv(std::vector<std::unique_ptr<details::CoroutineScheduler>>&& coroutine_schedulers,
                        std::vector<std::unique_ptr<details::EventScheduler>>&& event_schedulers)
{
    CoroutineSchedulerHolder::GetInstance()->Initialize(std::move(coroutine_schedulers));
    EventSchedulerHolder::GetInstance()->Initialize(std::move(event_schedulers));
}

void StartGalayEnv()
{
    CoroutineSchedulerHolder::GetInstance()->StartAll();
    EventSchedulerHolder::GetInstance()->StartAll();
}

void DestroyGalayEnv()
{
    CoroutineSchedulerHolder::GetInstance()->StopAll();
    EventSchedulerHolder::GetInstance()->StopAll();
}

}
