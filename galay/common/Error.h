#ifndef __GALAY_ERROR_H__
#define __GALAY_ERROR_H__

#include <cinttypes>
#include <string>
#include <memory>

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
    Error_SystemErrorEnd
};

enum HttpErrorCode
{
    kHttpError_NoError = 0,
    kHttpError_ConnectionClose = Error_SystemErrorEnd,
    kHttpError_RecvTimeOut,
    kHttpError_HeaderInComplete,
    kHttpError_BodyInComplete,
    kHttpError_HeaderTooLong,
    kHttpError_UriTooLong,
    kHttpError_ChunckHasError,
    kHttpError_HttpCodeInvalid,
    kHttpError_HeaderPairExist,
    kHttpError_HeaderPairNotExist,
    kHttpError_BadRequest,
    kHttpError_UrlInvalid,
    kHttpError_PortInvalid,
    kHttpError_UnkownError,
};

class HttpError
{
public:
    using ptr = std::shared_ptr<HttpError>;
    using wptr = std::weak_ptr<HttpError>;
    using uptr = std::unique_ptr<HttpError>;
    bool HasError() const;
    uint32_t& Code();
    void Reset();
    std::string ToString(uint32_t code) const;
protected:
    uint32_t m_code = kHttpError_NoError;
};


/*
    high 16bits for system error code
    lower 16bits for galay error code
*/
extern uint32_t MakeErrorCode(uint16_t galay_code, uint16_t system_code);

extern std::string GetErrorString(uint32_t code);

}

#endif