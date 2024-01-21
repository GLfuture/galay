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

    class Tcp_Request: public Tcp_Protocol,public Request_Base , public Response_Base
    {
    public:
        using ptr = std::shared_ptr<Tcp_Request>;
        //for server
        int decode(const std::string &buffer , int& state) override;

        int proto_fixed_len() override;

        int proto_extra_len() override;

        int proto_type() override;

        //for client
        std::string encode() override;

        void set_extra_msg(std::string&& msg) override;
    };

    class Tcp_Response: public Tcp_Protocol,public Request_Base , public Response_Base
    {
    public:
        using ptr = std::shared_ptr<Tcp_Response>;
        //for server
        std::string encode() override;

        int proto_type() override;

        int proto_fixed_len() override;

        int proto_extra_len() override;
        //for client 
        int decode(const std::string& buffer,int & state) override;

        void set_extra_msg(std::string&& msg) override;
    };

}

#endif