#include "Internal.h"
#include "Event.h"

namespace galay::details
{

bool AsyncTcpSocket(AsyncNetIo_wptr asocket)
{
    asocket.lock()->GetErrorCode() = error::ErrorCode::Error_NoError;
    asocket.lock()->GetHandle().fd = socket(AF_INET, SOCK_STREAM, 0);
    if (asocket.lock()->GetHandle().fd < 0) {
        asocket.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_SocketError, errno);
        return false;
    }
    HandleOption option(asocket.lock()->GetHandle());
    option.HandleNonBlock();
    return true;
}

bool AsyncUdpSocket(AsyncNetIo_wptr asocket)
{
    asocket.lock()->GetErrorCode() = error::ErrorCode::Error_NoError;
    asocket.lock()->GetHandle().fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (asocket.lock()->GetHandle().fd < 0) {
        asocket.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_SocketError, errno);
        return false;
    }
    HandleOption option(asocket.lock()->GetHandle());
    option.HandleNonBlock();
    return true;
}

bool Bind(AsyncNetIo_wptr asocket, const std::string& addr, int port)
{
    asocket.lock()->GetErrorCode() = error::ErrorCode::Error_NoError;
    sockaddr_in taddr{};
    taddr.sin_family = AF_INET;
    taddr.sin_port = htons(port);
    if(addr.empty()) taddr.sin_addr.s_addr = INADDR_ANY;
    else taddr.sin_addr.s_addr = inet_addr(addr.c_str());
    if( bind(asocket.lock()->GetHandle().fd, reinterpret_cast<sockaddr*>(&taddr), sizeof(taddr)) )
    {
        asocket.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_BindError, errno);
        return false;
    }
    return true;
}

bool Listen(AsyncNetIo_wptr asocket, int backlog)
{
    asocket.lock()->GetErrorCode() = error::ErrorCode::Error_NoError;
    if( listen(asocket.lock()->GetHandle().fd, backlog) )
    {
        asocket.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_ListenError, errno);
        return false;
    }
    return true;
}

coroutine::Awaiter_GHandle AsyncAccept(AsyncNetIo_wptr asocket, NetAddr* addr)
{
    dynamic_cast<NetWaitEvent*>(asocket.lock()->GetAction()->GetBindEvent())->ResetNetWaitEventType(kTcpWaitEventTypeAccept);
    asocket.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {asocket.lock()->GetAction(), addr};
}

coroutine::Awaiter_bool AsyncConnect(AsyncNetIo_wptr asocket, NetAddr* addr)
{
    dynamic_cast<NetWaitEvent*>(asocket.lock()->GetAction()->GetBindEvent())->ResetNetWaitEventType(kTcpWaitEventTypeConnect);
    asocket.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {asocket.lock()->GetAction(), addr};
}

coroutine::Awaiter_int AsyncRecv(AsyncNetIo_wptr asocket, TcpIOVec* iov, size_t length)
{
    dynamic_cast<NetWaitEvent*>(asocket.lock()->GetAction()->GetBindEvent())->ResetNetWaitEventType(kTcpWaitEventTypeRecv);
    asocket.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    iov->m_length = length;
    return {asocket.lock()->GetAction(), iov};
}

coroutine::Awaiter_int AsyncSend(AsyncNetIo_wptr asocket, TcpIOVec *iov, size_t length)
{
    dynamic_cast<NetWaitEvent*>(asocket.lock()->GetAction()->GetBindEvent())->ResetNetWaitEventType(kTcpWaitEventTypeSend);
    asocket.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    iov->m_length = length;
    return {asocket.lock()->GetAction(), iov};
}

coroutine::Awaiter_int AsyncRecvFrom(AsyncNetIo_wptr asocket, UdpIOVec *iov, size_t length)
{
    dynamic_cast<NetWaitEvent*>(asocket.lock()->GetAction()->GetBindEvent())->ResetNetWaitEventType(kUdpWaitEventTypeRecvFrom);
    asocket.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    iov->m_length = length;
    return coroutine::Awaiter_int(asocket.lock()->GetAction(), iov);
}

coroutine::Awaiter_int AsyncSendTo(AsyncNetIo_wptr asocket, UdpIOVec *iov, size_t length)
{
    dynamic_cast<NetWaitEvent*>(asocket.lock()->GetAction()->GetBindEvent())->ResetNetWaitEventType(kUdpWaitEventTypeSendTo);
    asocket.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    iov->m_length = length;
    return coroutine::Awaiter_int(asocket.lock()->GetAction(), iov);
}

coroutine::Awaiter_bool AsyncNetClose(AsyncNetIo_wptr asocket)
{
    dynamic_cast<NetWaitEvent*>(asocket.lock()->GetAction()->GetBindEvent())->ResetNetWaitEventType(kWaitEventTypeClose);
    asocket.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {asocket.lock()->GetAction(), nullptr};
}

bool AsyncSSLSocket(AsyncSslNetIo_wptr asocket, SSL_CTX *ctx)
{
    SSL* ssl = SSL_new(ctx);
    if(ssl == nullptr) return false;
    if(SSL_set_fd(ssl, asocket.lock()->GetHandle().fd)) {
        asocket.lock()->GetSSL() = ssl;
        return true;
    }
    SSL_free(ssl);
    return false;
}

coroutine::Awaiter_bool AsyncSSLAccept(AsyncSslNetIo_wptr asocket)
{
    dynamic_cast<NetWaitEvent*>(asocket.lock()->GetAction()->GetBindEvent())->ResetNetWaitEventType(kTcpWaitEventTypeSslAccept);
    asocket.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {asocket.lock()->GetAction(), nullptr};
}

coroutine::Awaiter_bool AsyncSSLConnect(AsyncSslNetIo_wptr asocket)
{
    dynamic_cast<NetWaitEvent*>(asocket.lock()->GetAction()->GetBindEvent())->ResetNetWaitEventType(kTcpWaitEventTypeSslConnect);
    asocket.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {asocket.lock()->GetAction(), nullptr};
}

coroutine::Awaiter_int AsyncSSLRecv(AsyncSslNetIo_wptr asocket, TcpIOVec *iov, size_t length)
{
    dynamic_cast<NetWaitEvent*>(asocket.lock()->GetAction()->GetBindEvent())->ResetNetWaitEventType(kTcpWaitEventTypeSslRecv);
    asocket.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    iov->m_length = length;
    return {asocket.lock()->GetAction(), iov};
}

coroutine::Awaiter_int AsyncSSLSend(AsyncSslNetIo_wptr asocket, TcpIOVec *iov, size_t length)
{
    dynamic_cast<NetWaitEvent*>(asocket.lock()->GetAction()->GetBindEvent())->ResetNetWaitEventType(kTcpWaitEventTypeSslSend);
    asocket.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    iov->m_length = length;
    return {asocket.lock()->GetAction(), iov};
}

coroutine::Awaiter_bool AsyncSSLClose(AsyncSslNetIo_wptr asocket)
{
    dynamic_cast<NetWaitEvent*>(asocket.lock()->GetAction()->GetBindEvent())->ResetNetWaitEventType(kWaitEventTypeSslClose);
    asocket.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {asocket.lock()->GetAction(), nullptr};
}

bool AsyncFileOpen(AsyncFileIo_wptr afile, const char *path, const int flags, mode_t mode)
{
#if defined(__linux__) || defined(__APPLE__) 
    const int fd = open(path, flags, mode);
    if( fd < 0 ) {
        return false;
    }
#endif
    afile.lock()->GetHandle() = GHandle{fd};
    HandleOption option(GHandle{fd});
    option.HandleNonBlock();
    return true;
}

coroutine::Awaiter_int AsyncFileRead(AsyncFileIo_wptr afile, FileIOVec *iov,  size_t length)
{
    dynamic_cast<FileIoWaitEvent*>(afile.lock()->GetAction()->GetBindEvent())->ResetFileIoWaitEventType(kFileIoWaitEventTypeRead);
    afile.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    iov->m_length = length;
    return {afile.lock()->GetAction(), iov};
}

coroutine::Awaiter_int AsyncFileWrite(AsyncFileIo_wptr afile, FileIOVec *iov, size_t length)
{
    dynamic_cast<FileIoWaitEvent*>(afile.lock()->GetAction()->GetBindEvent())->ResetFileIoWaitEventType(kFileIoWaitEventTypeWrite);
    afile.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    iov->m_length = length;
    return {afile.lock()->GetAction(), iov};
}

coroutine::Awaiter_bool AsyncFileClose(AsyncFileIo_wptr afile)
{
    dynamic_cast<FileIoWaitEvent*>(afile.lock()->GetAction()->GetBindEvent())->ResetFileIoWaitEventType(kFileIoWaitEventTypeClose);
    afile.lock()->GetErrorCode() = error::MakeErrorCode(error::ErrorCode::Error_NoError, 0);
    return {afile.lock()->GetAction(), nullptr};
}
}