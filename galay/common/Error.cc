#include "Error.h"
#include <string.h>

namespace galay::error
{

const char* g_error_str[] = {
    "No Error",
    "Socket Error",
    "Bind Error",
    "Listen Error",
    "Accept Error",
    "Connect Error",
    "Send Error",
    "Recv Error",
    "Close Error",
    "SSLSocket Error",
    "SSLAccept Error",
    "AsyncSSLConnect Error",
    "SSLClose Error",
    "FileRead Error",
    "FileWrite Error",
    "AddEvent Error",
    "ModEvent Error",
    "DelEvent Error",
    "GetSockName Error",
    "SetSockOpt Error",
    "SetBlock Error",
    "SetNoNlock Error",
    "Inet_Ntop Error",
    "EpollCreate Error",
    "EventWrite Error",
    "KQueueCreate Error",
    "LinuxAioSetup Error",
    "LinuxAioSubmit Error",


    "Connection Close",
    "Recv Timeout"
    "Header Incomplete",
    "Body Incomplete",
    "Header Too Long",
    "Uri Too Long",
    "Chunck Error",
    "Invalid Httpcode",
    "Header Pair Exist",
    "Header Pair Not Exist",
    "Bad Request",
    "Url Invalid",
    "Port Invalid",
    "Unkown Error"
};

uint32_t MakeErrorCode(uint16_t galay_code, uint16_t system_code)
{
    uint32_t ret = system_code << 16;
    return ret | galay_code;
}

std::string GetErrorString(uint32_t code)
{
    uint16_t galay_code = code & 0xFFFF;
    uint16_t system_code = code >> 16;
    std::string result = g_error_str[galay_code];
    result += ", system error: ";
    result += strerror(system_code);
    return std::move(result);
}


bool 
HttpError::HasError() const
{
    return this->m_code != error::kHttpError_NoError;
}

uint32_t& 
HttpError::Code()
{
    return this->m_code;
}

void HttpError::Reset()
{
    this->m_code = kHttpError_NoError;
}

std::string 
HttpError::ToString(const uint32_t code) const
{
    return GetErrorString(code);
}


    
}