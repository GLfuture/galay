#ifndef GALAY_OPERATION_H
#define GALAY_OPERATION_H

#include <any>
#include <functional>
#include "Coroutine.hpp"
#include "Async.hpp"
#include "galay/protocol/Protocol.h"
#include "galay/utils/Pool.hpp"

// namespace galay::details{
//     class EventScheduler;
// }

namespace galay
{

template <typename Socket>
class Connection: public std::enable_shared_from_this<Connection<Socket>>
{
public:
    using ptr = std::shared_ptr<Connection>;
    using EventScheduler = details::EventScheduler;
    using timeout_callback_t = std::function<void(Connection<Socket>::ptr)>;

    explicit Connection(EventScheduler* scheduler, Socket* socket);
    
    template <typename CoRtn = void>
    AsyncResult<int, CoRtn> Recv(TcpIOVecHolder& holder, int size, int64_t timeout_ms);

    template <typename CoRtn = void>
    AsyncResult<int, CoRtn> Send(TcpIOVecHolder& holder, int size, int64_t timeout_ms);

    template <typename CoRtn = void>
    AsyncResult<bool, CoRtn> Close();

    EventScheduler *GetScheduler() const;

    ~Connection();
private:
    Socket* m_socket;
    EventScheduler* m_scheduler;
};


template <typename T>
concept RequestType = std::is_base_of<Request, T>::value;

template <typename T>
concept ResponseType = std::is_base_of<Response, T>::value;

template <typename Socket, RequestType Request, ResponseType Response>
class Session
{
public:
    using ptr = std::shared_ptr<Session>;
    Session(Connection<Socket>::ptr connection);

    Connection<Socket>::ptr GetConnection();
    Request* GetRequest() const;
    Response* GetResponse() const;
    void* GetUserData();
    void SetUserData(void* data);

    void Close();
    bool IsClose();

    ~Session();
private:
    bool m_isClosed;
    void* m_userdata;
    Request* m_request;
    Response* m_response;
    Connection<Socket>::ptr m_connection;
};

}

#include "Session.tcc"

#endif