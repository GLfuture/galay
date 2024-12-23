#ifndef GALAY_INTERNAL_H
#define GALAY_INTERNAL_H

#include "Coroutine.hpp"
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

Awaiter<GHandle> AsyncAccept(AsyncNetIo_wptr asocket, NetAddr* addr);
Awaiter<bool> AsyncConnect(AsyncNetIo_wptr async_socket, NetAddr* addr);
/*
    return: 
        >0   bytes read
        0   close connection
        <0  error
*/
Awaiter<int> AsyncRecv(AsyncNetIo_wptr asocket, TcpIOVec* iov, size_t length);
/*
    return: 
        >0   bytes send
        <0  error
*/
Awaiter<int> AsyncSend(AsyncNetIo_wptr asocket, TcpIOVec* iov, size_t length);
/*

*/
Awaiter<int> AsyncRecvFrom(AsyncNetIo_wptr asocket, UdpIOVec* iov, size_t length);
Awaiter<int> AsyncSendTo(AsyncNetIo_wptr asocket, UdpIOVec* iov, size_t length);
Awaiter<bool> AsyncNetClose(AsyncNetIo_wptr asocket);

bool AsyncSSLSocket(AsyncSslNetIo_wptr asocket, SSL_CTX* ctx);
/*
    must be called after AsyncAccept
*/
Awaiter<bool> AsyncSSLAccept(AsyncSslNetIo_wptr asocket);
/*
    must be called after AsyncConnect
*/
Awaiter<bool> AsyncSSLConnect(AsyncSslNetIo_wptr asocket);
Awaiter<int> AsyncSSLRecv(AsyncSslNetIo_wptr asocket, TcpIOVec *iov, size_t length);
Awaiter<int> AsyncSSLSend(AsyncSslNetIo_wptr asocket, TcpIOVec *iov, size_t length);
Awaiter<bool> AsyncSSLClose(AsyncSslNetIo_wptr asocket);

/*
    ****************************
                file 
    ****************************
*/
bool AsyncFileOpen(AsyncFileIo_wptr afileio, const char* path, int flags, mode_t mode);
Awaiter<int> AsyncFileRead(AsyncFileIo_wptr afile, FileIOVec* iov, size_t length);
Awaiter<int> AsyncFileWrite(AsyncFileIo_wptr afile, FileIOVec* iov, size_t length);
Awaiter<bool> AsyncFileClose(AsyncFileIo_wptr afile);


}


#endif