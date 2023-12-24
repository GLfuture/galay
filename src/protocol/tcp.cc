#include "tcp.h"

int galay::Tcp_Request::decode(const std::string &buffer , int& state)
{
    this->m_buffer = buffer;
    return this->m_buffer.length();
}

std::string& galay::Tcp_Protocol::get_buffer() 
{
    return this->m_buffer;
}

std::string galay::Tcp_Response::encode()
{
    return this->m_buffer;
}