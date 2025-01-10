#ifndef GALAY_SESSION_TCC
#define GALAY_SESSION_TCC

#include "Session.hpp"

namespace galay
{

template<typename Socket>
Connection<Socket>::Connection(Socket* socket) 
    :m_socket(socket) 
{
}

template<typename Socket>
Socket* Connection<Socket>::GetSocket() const
{
    return m_socket;
}

template<typename Socket>
Connection<Socket>::~Connection() 
{ 
    delete m_socket; 
}


template <typename Socket>
CallbackStore<Socket>::CallbackStore(const std::function<Coroutine<void>(RoutineCtx,std::shared_ptr<Connection<Socket>>)>& callback) 
    : m_callback(callback) 
{

}

template <typename Socket>
void CallbackStore<Socket>::Execute(Socket* socket) 
{
    auto connection = std::make_shared<Connection<Socket>>(socket);
    m_callback(galay::RoutineCtx::Create(), connection);
}


template <typename Socket, RequestType Request, ResponseType Response>
Session<Socket, Request, Response>::Session(std::shared_ptr<Connection<Socket>> connection)
        :m_connection(connection), m_request(std::make_shared<Request>()), m_response(std::make_shared<Response>())
{

}

template <typename Socket, RequestType Request, ResponseType Response>
Request* Session<Socket, Request, Response>::GetRequest() const 
{ 
    return m_request.get(); 
}

template <typename Socket, RequestType Request, ResponseType Response>
Response* Session<Socket, Request, Response>::GetResponse() const 
{ 
    return m_response.get(); 
}

template <typename Socket, RequestType Request, ResponseType Response>
std::shared_ptr<Connection<Socket>> Session<Socket, Request, Response>::GetConnection() 
{ 
    return m_connection; 
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
void Session<Socket, Request, Response>::ToClose() 
{ 
    m_close = true; 
}

template <typename Socket, RequestType Request, ResponseType Response>
bool Session<Socket, Request, Response>::IsClose() 
{ 
    return m_close; 
}


}


#endif