#include "tcp.h"

std::string& galay::Tcp_Protocol::get_buffer() 
{
    return this->m_buffer;
}

int galay::Tcp_Request::decode(const std::string &buffer , int& state)
{
    this->m_buffer = buffer;
    state = error::base_error::GY_SUCCESS;
    return this->m_buffer.length();
}

int galay::Tcp_Request::proto_type()
{
    return GY_ALL_RECIEVE_PROTOCOL_TYPE;
}

int galay::Tcp_Request::proto_fixed_len()
{
    return 0;
}

int galay::Tcp_Request::proto_extra_len()
{
    return 0;
}

void galay::Tcp_Request::set_extra_msg(std::string&& msg)
{

}

std::string galay::Tcp_Request::encode()
{
    return this->m_buffer;
}

std::string galay::Tcp_Response::encode()
{
    return this->m_buffer;
}

int galay::Tcp_Response::decode(const std::string& buffer,int & state)
{
    this->m_buffer = buffer;
    state = error::base_error::GY_SUCCESS;
    return this->m_buffer.length();
}

int galay::Tcp_Response::proto_type()
{
    return GY_ALL_RECIEVE_PROTOCOL_TYPE;
}

int galay::Tcp_Response::proto_fixed_len()
{
    return 0;
}

int galay::Tcp_Response::proto_extra_len()
{
    return 0;
}

void galay::Tcp_Response::set_extra_msg(std::string&& msg)
{

}