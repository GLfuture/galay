#ifndef GALAY_BASIC_PROTOCOL_H
#define GALAY_BASIC_PROTOCOL_H

#include <string>
#include <memory>
#include "../kernel/basic_concepts.h"

namespace galay
{
    class Tcp_Request_Base
    {
    public:
        using ptr = std::shared_ptr<Tcp_Request_Base>;
        virtual int DecodePdu(const std::string &buffer) = 0;
        virtual Proto_Judge_Type IsPduAndLegal(const std::string& buffer) = 0;
    };

    class Tcp_Response_Base
    {
    public:
        using ptr = std::shared_ptr<Tcp_Response_Base>;
        virtual std::string EncodePdu() = 0;
    };

    class Udp_Request_Base
    {
    public:
        using ptr = std::shared_ptr<Udp_Request_Base>;
        virtual int DecodePdu(const std::string& buffer) = 0;
    };

    class Udp_Response_Base
    {
    public:
        using ptr = std::shared_ptr<Udp_Response_Base>;
        virtual std::string EncodePdu() = 0;
    };
}


#endif