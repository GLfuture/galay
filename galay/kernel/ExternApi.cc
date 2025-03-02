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

bool InitialiszeSSLClientEnv(const char* server_pem)
{
    SslCtx = SSL_CTX_new(TLS_client_method());
    if (SslCtx == nullptr) {
        return false;
    }
    if(server_pem) {
        if(SSL_CTX_load_verify_locations(SslCtx, server_pem, NULL) <= 0) {
            return false;
        }
    }
    return true;
}

bool DestroySSLEnv()
{
    SSL_CTX_free(SslCtx);
    EVP_cleanup();
    return true;
}

bool InitializeHttp2ServerEnv(const char* cert_file, const char* key_file)
{
    if(!InitializeSSLServerEnv(cert_file, key_file)) {
        return false;
    }
    const unsigned char alpn_protocols[] = "\x08\x04\x00\x00"; // HTTP/2
    SSL_CTX_set_alpn_protos(SslCtx, alpn_protocols, sizeof(alpn_protocols));
    return true;
}

bool InitializeHttp2ClientEnv(const char* server_pem)
{
    if(!InitialiszeSSLClientEnv(server_pem)) {
        return false;
    }
    const unsigned char alpn_protocols[] = "\x08\x04\x00\x00"; // HTTP/2
    SSL_CTX_set_alpn_protos(SslCtx, alpn_protocols, sizeof(alpn_protocols));
    return true;
}

bool DestroyHttp2Env()
{
    DestroySSLEnv();
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
