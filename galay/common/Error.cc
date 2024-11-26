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
    "SSLConnect Error",
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
};

uint32_t MakeErrorCode(uint16_t galay_code, uint16_t system_code)
{
    uint32_t ret = galay_code << 16;
    return ret | system_code;
}

std::string GetErrorString(uint32_t code)
{
    uint16_t galay_code = code >> 16;
    uint16_t system_code = code & 0xFFFF;
    std::string result = g_error_str[galay_code];
    result += ", system error: ";
    result += strerror(system_code);
    return std::move(result);
}

    
}