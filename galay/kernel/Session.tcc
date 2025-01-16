#ifndef GALAY_SESSION_TCC
#define GALAY_SESSION_TCC

#include "Session.hpp"
#include <iostream>

namespace galay
{

template<typename Socket>
Connection<Socket>::Connection(Socket* socket) 
    :m_socket(socket) 
{
}

template<typename Socket>
template <typename CoRtn>
AsyncResult<int, CoRtn> Connection<Socket>::Recv(TcpIOVec *iov, int size)
{
    return m_socket->Recv(iov, size);
}

template <typename Socket>
template <typename CoRtn>
inline AsyncResult<int, CoRtn> Connection<Socket>::Send(TcpIOVec *iov, int size)
{
    return m_socket->Send(iov, size);
}

template <typename Socket>
template <typename CoRtn>
inline AsyncResult<bool, CoRtn> Connection<Socket>::Close()
{
    return m_socket->Close();
}


template <typename Socket>
inline void Connection<Socket>::Destroy()
{
    delete this;
}

template<typename Socket>
Connection<Socket>::~Connection() 
{ 
    delete m_socket; 
}


template <typename Socket, RequestType Request, ResponseType Response>
Session<Socket, Request, Response>::Session(Connection<Socket>::ptr connection)
        :m_connection(connection), m_request(new Request), m_response(new Response), m_isClosed(false)
{

}

template <typename Socket, RequestType Request, ResponseType Response>
inline Connection<Socket>::ptr Session<Socket, Request, Response>::GetConnection()
{
    return m_connection;
}

template <typename Socket, RequestType Request, ResponseType Response>
Request *Session<Socket, Request, Response>::GetRequest() const
{ 
    return m_request; 
}

template <typename Socket, RequestType Request, ResponseType Response>
Response* Session<Socket, Request, Response>::GetResponse() const 
{ 
    return m_response; 
}


template <typename Socket, RequestType Request, ResponseType Response>
void* Session<Socket, Request, Response>::GetUserData() 
{ 
    return m_userdata; 
}

template <typename Socket, RequestType Request, ResponseType Response>
void Session<Socket, Request, Response>::SetUserData(void* data) 
{
    m_userdata = data; 
}


template <typename Socket, RequestType Request, ResponseType Response>
inline void Session<Socket, Request, Response>::Close()
{
    m_isClosed = true;
}


template <typename Socket, RequestType Request, ResponseType Response>
inline bool Session<Socket, Request, Response>::IsClose()
{
    return m_isClosed;
}

template <typename Socket, RequestType Request, ResponseType Response>
inline Session<Socket, Request, Response>::~Session()
{
    delete m_request;
    delete m_response;
}


}

#endif