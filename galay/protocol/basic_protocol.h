#ifndef GALAY_BASIC_PROTOCOL_H
#define GALAY_BASIC_PROTOCOL_H

#include <string>
#include <memory>

namespace galay
{
    enum Proto_Type{
        GY_PACKAGE_FIXED_PROTOCOL_TYPE,  //固定长度包的协议
        GY_HEAD_FIXED_PROTOCOL_TYPE,     //固定头长的协议,头中含有length字段
        GY_ALL_RECIEVE_PROTOCOL_TYPE,    //把读缓冲区读空，适用于一请求一回应
    };


    class Request_Base
    {
    public:
        using ptr = std::shared_ptr<Request_Base>;
        virtual int decode(const std::string &buffer , int &state) = 0;
        virtual int proto_type() = 0;
        virtual int proto_fixed_len() = 0;
        virtual int proto_extra_len() = 0;
        virtual void set_extra_msg(std::string&& msg) = 0;
    };

    class Response_Base
    {
    public:
        using ptr = std::shared_ptr<Response_Base>;
        virtual std::string encode() = 0;
    };
}


#endif