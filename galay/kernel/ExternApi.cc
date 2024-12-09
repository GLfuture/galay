#include "ExternApi.h"
#include "Scheduler.h"
#include "Async.h"
#include "Event.h"
#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    #include <fcntl.h>
#endif

namespace galay
{

SSL_CTX* g_ssl_ctx = nullptr;


IOVecHolder::IOVecHolder(size_t size)
{
    VecMalloc(&m_vec, size);
}

IOVecHolder::IOVecHolder()
{
}

void IOVecHolder::Realloc(size_t size)
{
    VecRealloc(&m_vec, size);
}

void IOVecHolder::ClearBuffer()
{
    if(m_vec.m_buffer == nullptr) return;
    memset(m_vec.m_buffer, 0, m_vec.m_size);
    m_vec.m_offset = 0;
    m_vec.m_length = 0;
}

bool IOVecHolder::Reset(const IOVec &iov)
{
    if(iov.m_buffer == nullptr) return false;
    FreeMemory();
    m_vec.m_buffer = iov.m_buffer;
    m_vec.m_size = iov.m_size;
    m_vec.m_offset = iov.m_offset;
    m_vec.m_length = iov.m_length;
    return true;
}

bool IOVecHolder::Reset()
{
    return FreeMemory();
}

bool IOVecHolder::Reset(std::string &&str)
{
    FreeMemory();
    m_temp = std::forward<std::string>(str);
    m_vec.m_buffer = m_temp.data();
    m_vec.m_size = m_temp.length();
    return true;
}

IOVec *IOVecHolder::operator->()
{
    return &m_vec;
}

IOVec *IOVecHolder::operator&()
{
    return &m_vec;
}

IOVecHolder::~IOVecHolder()
{
    FreeMemory();    
}

bool IOVecHolder::FreeMemory()
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
// API

void VecFull(IOVec *src, char *buffer, size_t size, size_t offset, size_t length)
{
    src->m_buffer = buffer;
    src->m_size = size;
    src->m_offset = offset;
    src->m_length = length;
}

IOVec VecDeepCopy(const IOVec *src)
{
    if(src == nullptr || src->m_length == 0 || src->m_buffer == nullptr || src->m_size == 0) {
        return IOVec{};
    }
    char* buffer = static_cast<char*>(calloc(src->m_length, sizeof(char)));
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
    src->m_buffer = static_cast<char*>(calloc(size, sizeof(char)));
    if (src->m_buffer == nullptr) {
        return false;
    }
    src->m_size = size;
    return true;
}

bool VecRealloc(IOVec *src, size_t size)
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

bool VecFree(IOVec *src)
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
    dynamic_cast<details::NetWaitEvent*>(asocket->GetAction()->GetBindEvent())->ResetNetWaitEventType(details::kTcpWaitEventTypeAccept);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {asocket->GetAction(), addr};
}

coroutine::Awaiter_bool AsyncConnect(async::AsyncNetIo *asocket, NetAddr* addr)
{
    dynamic_cast<details::NetWaitEvent*>(asocket->GetAction()->GetBindEvent())->ResetNetWaitEventType(details::kTcpWaitEventTypeConnect);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {asocket->GetAction(), addr};
}

coroutine::Awaiter_int AsyncRecv(async::AsyncNetIo *asocket, IOVec* iov, size_t length)
{
    dynamic_cast<details::NetWaitEvent*>(asocket->GetAction()->GetBindEvent())->ResetNetWaitEventType(details::kTcpWaitEventTypeRecv);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    iov->m_length = length;
    return {asocket->GetAction(), iov};
}

coroutine::Awaiter_int AsyncSend(async::AsyncNetIo *asocket, IOVec *iov, size_t length)
{
    dynamic_cast<details::NetWaitEvent*>(asocket->GetAction()->GetBindEvent())->ResetNetWaitEventType(details::kTcpWaitEventTypeSend);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    iov->m_length = length;
    return {asocket->GetAction(), iov};
}

coroutine::Awaiter_bool AsyncClose(async::AsyncNetIo *asocket)
{
    dynamic_cast<details::NetWaitEvent*>(asocket->GetAction()->GetBindEvent())->ResetNetWaitEventType(details::kTcpWaitEventTypeClose);
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
    dynamic_cast<details::NetWaitEvent*>(asocket->GetAction()->GetBindEvent())->ResetNetWaitEventType(details::kTcpWaitEventTypeSslAccept);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {asocket->GetAction(), nullptr};
}

coroutine::Awaiter_bool SSLConnect(async::AsyncSslNetIo *asocket)
{
    dynamic_cast<details::NetWaitEvent*>(asocket->GetAction()->GetBindEvent())->ResetNetWaitEventType(details::kTcpWaitEventTypeSslConnect);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {asocket->GetAction(), nullptr};
}

coroutine::Awaiter_int AsyncSSLRecv(async::AsyncSslNetIo *asocket, IOVec *iov, size_t length)
{
    dynamic_cast<details::NetWaitEvent*>(asocket->GetAction()->GetBindEvent())->ResetNetWaitEventType(details::kTcpWaitEventTypeSslRecv);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    iov->m_length = length;
    return {asocket->GetAction(), iov};
}

coroutine::Awaiter_int AsyncSSLSend(async::AsyncSslNetIo *asocket, IOVec *iov, size_t length)
{
    dynamic_cast<details::NetWaitEvent*>(asocket->GetAction()->GetBindEvent())->ResetNetWaitEventType(details::kTcpWaitEventTypeSslSend);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    iov->m_length = length;
    return {asocket->GetAction(), iov};
}

coroutine::Awaiter_bool AsyncSSLClose(async::AsyncSslNetIo *asocket)
{
    dynamic_cast<details::NetWaitEvent*>(asocket->GetAction()->GetBindEvent())->ResetNetWaitEventType(details::kTcpWaitEventTypeSslClose);
    asocket->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {asocket->GetAction(), nullptr};
}

coroutine::Awaiter_GHandle AsyncFileOpen(const char *path, const int flags, mode_t mode)
{
#if defined(__linux__) || defined(__APPLE__) 
    const int fd = open(path, flags, mode);
#endif
    const auto handle = GHandle{fd};
    async::HandleOption option(handle);
    option.HandleNonBlock();
    return coroutine::Awaiter_GHandle(GHandle{fd});
}

coroutine::Awaiter_int AsyncFileRead(async::AsyncFileIo *afileio, IOVec *iov, size_t length)
{
    dynamic_cast<details::FileIoWaitEvent*>(afileio->GetAction()->GetBindEvent())->ResetFileIoWaitEventType(details::kFileIoWaitEventTypeRead);
    afileio->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    iov->m_length = length;
    return {afileio->GetAction(), iov};
}

coroutine::Awaiter_int AsyncFileWrite(async::AsyncFileIo *afileio, IOVec *iov, size_t length)
{
    dynamic_cast<details::FileIoWaitEvent*>(afileio->GetAction()->GetBindEvent())->ResetFileIoWaitEventType(details::kFileIoWaitEventTypeWrite);
    afileio->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    iov->m_length = length;
    return {afileio->GetAction(), iov};
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
