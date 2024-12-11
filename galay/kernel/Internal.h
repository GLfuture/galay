#ifndef GALAY_INTERNAL_H
#define GALAY_INTERNAL_H

#include "Awaiter.h"
#include "Async.h"
#include "ExternApi.h"

namespace galay::details 
{

/*
    ****************************
                net
    ****************************
*/

using AsyncNetIo_wptr = std::weak_ptr<AsyncNetIo>;
using AsyncSslNetIo_wptr = std::weak_ptr<AsyncSslNetIo>;
using AsyncFileIo_wptr = std::weak_ptr<AsyncFileIo>;

bool AsyncTcpSocket(AsyncNetIo_wptr asocket);
bool AsyncUdpSocket(AsyncNetIo_wptr asocket);
bool Bind(AsyncNetIo_wptr asocket, const std::string& addr, int port);
bool Listen(AsyncNetIo_wptr asocket, int backlog);

coroutine::Awaiter_GHandle AsyncAccept(AsyncNetIo_wptr asocket, NetAddr* addr);
coroutine::Awaiter_bool AsyncConnect(AsyncNetIo_wptr async_socket, NetAddr* addr);
/*
    return: 
        >0   bytes read
        0   close connection
        <0  error
*/
coroutine::Awaiter_int AsyncRecv(AsyncNetIo_wptr asocket, TcpIOVec* iov, size_t length);
/*
    return: 
        >0   bytes send
        <0  error
*/
coroutine::Awaiter_int AsyncSend(AsyncNetIo_wptr asocket, TcpIOVec* iov, size_t length);
/*

*/
coroutine::Awaiter_int AsyncRecvFrom(AsyncNetIo_wptr asocket, UdpIOVec* iov, size_t length);
coroutine::Awaiter_int AsyncSendTo(AsyncNetIo_wptr asocket, UdpIOVec* iov, size_t length);
coroutine::Awaiter_bool AsyncNetClose(AsyncNetIo_wptr asocket);

bool AsyncSSLSocket(AsyncSslNetIo_wptr asocket, SSL_CTX* ctx);
/*
    must be called after AsyncAccept
*/
coroutine::Awaiter_bool AsyncSSLAccept(AsyncSslNetIo_wptr asocket);
/*
    must be called after AsyncConnect
*/
coroutine::Awaiter_bool AsyncSSLConnect(AsyncSslNetIo_wptr asocket);
coroutine::Awaiter_int AsyncSSLRecv(AsyncSslNetIo_wptr asocket, TcpIOVec *iov, size_t length);
coroutine::Awaiter_int AsyncSSLSend(AsyncSslNetIo_wptr asocket, TcpIOVec *iov, size_t length);
coroutine::Awaiter_bool AsyncSSLClose(AsyncSslNetIo_wptr asocket);

/*
    ****************************
                file 
    ****************************
*/
bool AsyncFileOpen(AsyncFileIo_wptr afileio, const char* path, int flags, mode_t mode);
coroutine::Awaiter_int AsyncFileRead(AsyncFileIo_wptr afile, FileIOVec* iov, size_t length);
coroutine::Awaiter_int AsyncFileWrite(AsyncFileIo_wptr afile, FileIOVec* iov, size_t length);
coroutine::Awaiter_bool AsyncFileClose(AsyncFileIo_wptr afile);


}


#endif