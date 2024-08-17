#include "protocol.h"

std::any 
galay::protocol::GY_Request::Error()
{
    return this->m_error;
}

bool 
galay::protocol::GY_Request::ParseIncomplete()
{
    return this->m_parseStatus == kProtoParseIncomplete;
}
bool 
galay::protocol::GY_Request::ParseIllegal()
{
    return this->m_parseStatus == kProtoParseIllegal;
}

bool 
galay::protocol::GY_Request::ParseSuccess()
{
    return this->m_parseStatus == kProtoParseSuccess;
}

void 
galay::protocol::GY_Request::SetErrorContext(std::any error)
{
    this->m_error = error;
}

void 
galay::protocol::GY_Request::Incomplete() 
{
    this->m_parseStatus = kProtoParseIncomplete;
}

void 
galay::protocol::GY_Request::Illegal() 
{
    this->m_parseStatus = kProtoParseIllegal;
}

void 
galay::protocol::GY_Request::Success() 
{
    this->m_parseStatus = kProtoParseSuccess;
}

std::any 
galay::protocol::GY_Response::Error()
{
    return this->m_error;
}

bool 
galay::protocol::GY_Response::ParseIncomplete()
{
    return this->m_parseStatus == kProtoParseIncomplete;
}
bool 
galay::protocol::GY_Response::ParseIllegal()
{
    return this->m_parseStatus == kProtoParseIllegal;
}

bool 
galay::protocol::GY_Response::ParseSuccess()
{
    return this->m_parseStatus == kProtoParseSuccess;
}

void 
galay::protocol::GY_Response::SetErrorContext(std::any error)
{
    this->m_error = error;
}

void 
galay::protocol::GY_Response::Incomplete() 
{
    this->m_parseStatus = kProtoParseIncomplete;
}

void 
galay::protocol::GY_Response::Illegal() 
{
    this->m_parseStatus = kProtoParseIllegal;
}

void 
galay::protocol::GY_Response::Success() 
{
    this->m_parseStatus = kProtoParseSuccess;
}