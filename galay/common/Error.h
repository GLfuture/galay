#ifndef __GALAY_ERROR_H__
#define __GALAY_ERROR_H__

#include <cinttypes>
#include <string>

namespace galay::error
{
    
enum ErrorCode
{
    Error_NoError,
    Error_SocketError,
    Error_BindError,
    Error_ListenError,
    Error_AcceptError,
    Error_ConnectError,
    Error_RecvError,
    Error_SendError,
    Error_CloseError,
    Error_SSLSocketError,
    Error_SSLAcceptError,
    Error_SSLConnectError,
    Error_SSLCloseError,
    Error_FileReadError,
    Error_FileWriteError,
    Error_AddEventError,
    Error_ModEventError,
    Error_DelEventError,
    Error_GetSockNameError,
    Error_SetSockOptError,
    Error_SetBlockError,
    Error_SetNoBlockError,
    Error_InetNtopError,
    Error_EpollCreateError,
    Error_EventWriteError,
    Error_KqueueCreateError,
    Error_LinuxAioSetupError,
    Error_LinuxAioSubmitError,
};

/*
    16bits for galay error code
    16bits for system error code
*/
extern uint32_t MakeErrorCode(uint16_t galay_code, uint16_t system_code);

extern std::string GetErrorString(uint32_t code);

}

#endif