#include "Protocol.h"

namespace galay::protocol
{

bool 
Request::ParseIncomplete()
{
    return this->m_parseStatus == kProtoParseIncomplete;
}
bool 
Request::ParseIllegal()
{
    return this->m_parseStatus == kProtoParseIllegal;
}

bool 
Request::ParseSuccess()
{
    return this->m_parseStatus == kProtoParseSuccess;
}


void 
Request::Incomplete() 
{
    this->m_parseStatus = kProtoParseIncomplete;
}

void 
Request::Illegal() 
{
    this->m_parseStatus = kProtoParseIllegal;
}

void 
Request::Success() 
{
    this->m_parseStatus = kProtoParseSuccess;
}

bool 
Response::ParseIncomplete()
{
    return this->m_parseStatus == kProtoParseIncomplete;
}
bool 
Response::ParseIllegal()
{
    return this->m_parseStatus == kProtoParseIllegal;
}

bool 
Response::ParseSuccess()
{
    return this->m_parseStatus == kProtoParseSuccess;
}

void 
Response::Incomplete() 
{
    this->m_parseStatus = kProtoParseIncomplete;
}

void 
Response::Illegal() 
{
    this->m_parseStatus = kProtoParseIllegal;
}

void 
Response::Success() 
{
    this->m_parseStatus = kProtoParseSuccess;
}
}