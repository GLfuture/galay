#ifndef GALAY_TCP_H
#define GALAY_TCP_H

#include "basic_protocol.h"
#include "../kernel/error.h"

namespace galay
{
    class Tcp_Protocol
    {
    public:
        std::string& get_buffer();
    protected:
        std::string m_buffer;
    };

    class Tcp_Request: public Tcp_Protocol,public Request_Base
    {
    public:
        using ptr = std::shared_ptr<Tcp_Request>;
        int decode(const std::string &buffer , int& state);

    };

    class Tcp_Response: public Tcp_Protocol,public Response_Base
    {
    public:
        using ptr = std::shared_ptr<Tcp_Response>;
        std::string encode();
    };

}

#endif