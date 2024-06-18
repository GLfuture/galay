#ifndef GALAY_BASIC_PROTOCOL_H
#define GALAY_BASIC_PROTOCOL_H

#include <string>
#include <memory>
#include "../kernel/base.h"

namespace galay
{
    namespace protocol
    {
        class GY_TcpRequest
        {
        public:
            using ptr = std::shared_ptr<GY_TcpRequest>;
            using wptr = std::weak_ptr<GY_TcpRequest>;
            using uptr = std::unique_ptr<GY_TcpRequest>;
            virtual ProtoJudgeType DecodePdu(std::string &buffer) = 0;
            virtual void Clear() = 0;
        };

        class GY_TcpResponse
        {
        public:
            using ptr = std::shared_ptr<GY_TcpResponse>;
            using wptr = std::weak_ptr<GY_TcpResponse>;
            using uptr = std::unique_ptr<GY_TcpResponse>;
            virtual std::string EncodePdu() = 0;
            virtual void Clear() = 0;
        };

        class GY_UdpRequest
        {
        public:
            using ptr = std::shared_ptr<GY_UdpRequest>;
            virtual int DecodePdu(std::string &buffer) = 0;
            virtual void Clear() = 0;
        };

        class GY_UdpResponse
        {
        public:
            using ptr = std::shared_ptr<GY_UdpResponse>;
            virtual std::string EncodePdu() = 0;
            virtual void Clear() = 0;
        };
    }
}

#endif