#ifndef GALAY_HTTP_SERVER_HPP
#define GALAY_HTTP_SERVER_HPP

#include "server.hpp"

namespace galay
{
    template <Request REQ, Response REPS>
    class Http_Server : public Tcp_Server<REQ,RESP>
    {
    public:
        
    };
}
#endif