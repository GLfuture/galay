#ifndef GALAY_HTTP2_H
#define GALAY_HTTP2_H

#include "Protocol.h"

namespace galay::http2
{

class Http2_0 final : public Request, public Response
{
public:
    Http2_0() = default;
    ~Http2_0() = default;

    virtual std::pair<bool,size_t> DecodePdu(const std::string_view &buffer) override;
    virtual std::string EncodePdu() const override;
    virtual bool HasError() const override;
    virtual int GetErrorCode() const override;
    virtual std::string GetErrorString() override;
    virtual void Reset() override;
};




}


#endif