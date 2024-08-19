#include "protocol.h"

namespace galay::protocol
{

bool 
GY_Request::ParseIncomplete()
{
    return this->m_parseStatus == kProtoParseIncomplete;
}
bool 
GY_Request::ParseIllegal()
{
    return this->m_parseStatus == kProtoParseIllegal;
}

bool 
GY_Request::ParseSuccess()
{
    return this->m_parseStatus == kProtoParseSuccess;
}


void 
GY_Request::Incomplete() 
{
    this->m_parseStatus = kProtoParseIncomplete;
}

void 
GY_Request::Illegal() 
{
    this->m_parseStatus = kProtoParseIllegal;
}

void 
GY_Request::Success() 
{
    this->m_parseStatus = kProtoParseSuccess;
}

bool 
GY_Response::ParseIncomplete()
{
    return this->m_parseStatus == kProtoParseIncomplete;
}
bool 
GY_Response::ParseIllegal()
{
    return this->m_parseStatus == kProtoParseIllegal;
}

bool 
GY_Response::ParseSuccess()
{
    return this->m_parseStatus == kProtoParseSuccess;
}

void 
GY_Response::Incomplete() 
{
    this->m_parseStatus = kProtoParseIncomplete;
}

void 
GY_Response::Illegal() 
{
    this->m_parseStatus = kProtoParseIllegal;
}

void 
GY_Response::Success() 
{
    this->m_parseStatus = kProtoParseSuccess;
}
}