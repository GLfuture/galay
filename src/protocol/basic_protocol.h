#ifndef GALAY_BASIC_PROTOCOL_H
#define GALAY_BASIC_PROTOCOL_H

#include <string>
#include <memory>

namespace galay
{
    class Request_Base
    {
    public:
        using ptr = std::shared_ptr<Request_Base>;
        virtual int decode(const std::string &buffer , int &state) = 0;
    };

    class Response_Base
    {
    public:
        using ptr = std::shared_ptr<Response_Base>;
        virtual std::string encode() = 0;
    };
}


#endif