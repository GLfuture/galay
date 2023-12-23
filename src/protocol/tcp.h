#ifndef GALAY_TCP_H
#define GALAY_TCP_H

#include "basic_protocol.h"

namespace galay
{
    class Tcp_Protocol
    {
    protected:
        std::string m_buffer;
    };

    class Tcp_Request: public Tcp_Protocol,public Request_Base
    {
    public:
        using ptr = std::shared_ptr<Tcp_Request>;
        int decode(const std::string &buffer , int& state)
        {
            this->m_buffer = buffer;
            return this->m_buffer.length();
        }

        std::string get_buffer() const {
            return this->m_buffer;
        }

    };

    class Tcp_Response: public Tcp_Protocol,public Response_Base
    {
    public:
        using ptr = std::shared_ptr<Tcp_Response>;
        std::string encode()
        {
            return this->m_buffer;
        }

        void set_buffer(const std::string& buffer){
            this->m_buffer = buffer;
        }

    };

}

#endif