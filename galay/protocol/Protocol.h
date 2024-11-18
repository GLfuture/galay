#ifndef GALAY_PROTOCOL_H
#define GALAY_PROTOCOL_H

#include <memory>
#include <string>
#include <string_view>
#include "galay/common/Reflection.h"

namespace galay::protocol
{
    class SRequest{
    public:
        using ptr = std::shared_ptr<SRequest>;
        using wptr = std::weak_ptr<SRequest>;
        using uptr = std::unique_ptr<SRequest>;
        virtual int DecodePdu(const std::string_view &buffer) = 0;
    };

    class CResponse{
    public:
        using ptr = std::shared_ptr<CResponse>;
        using wptr = std::weak_ptr<CResponse>;
        using uptr = std::unique_ptr<CResponse>;
        virtual int DecodePdu(const std::string_view &buffer) = 0;
    };

    class SResponse{
    public:
        using ptr = std::shared_ptr<SResponse>;
        using wptr = std::weak_ptr<SResponse>;
        using uptr = std::unique_ptr<SResponse>;
        virtual std::string EncodePdu() = 0;
    };
    
    class CRequest{
    public:
        using ptr = std::shared_ptr<CRequest>;
        using wptr = std::weak_ptr<CRequest>;
        using uptr = std::unique_ptr<CRequest>;
        virtual std::string EncodePdu() = 0;
    };

    class Request: public SRequest, public CRequest
    {
    public:
        using ptr = std::shared_ptr<Request>;
        using wptr = std::weak_ptr<Request>;
        using uptr = std::unique_ptr<Request>;
        virtual int DecodePdu(const std::string_view &buffer) = 0;
        virtual std::string EncodePdu() = 0;
        virtual bool HasError() const = 0;
        virtual int GetErrorCode() const = 0;
        virtual std::string GetErrorString() = 0;
        virtual void Reset() = 0;
        
    };

    class Response: public SResponse, public CResponse
    {
    public:
        using ptr = std::shared_ptr<Response>;
        using wptr = std::weak_ptr<Response>;
        using uptr = std::unique_ptr<Response>;
        virtual int DecodePdu(const std::string_view &buffer) = 0;
        virtual std::string EncodePdu() = 0;
        virtual bool HasError() const = 0;
        virtual int GetErrorCode() const = 0;
        virtual std::string GetErrorString() = 0;
        virtual void Reset() = 0;
    };
    
}

#endif