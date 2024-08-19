#ifndef GALAY_PROTOCOL_H
#define GALAY_PROTOCOL_H

#include <string>
#include <memory>
#include <any>
#include "../common/reflection.h"

namespace galay
{
    namespace protocol
    {
        enum ProtoParseType
        {
            kProtoParseSuccess,
            kProtoParseIncomplete,
            kProtoParseIllegal,
        };

        class GY_SRequest{
        public:
            using ptr = std::shared_ptr<GY_SRequest>;
            using wptr = std::weak_ptr<GY_SRequest>;
            using uptr = std::unique_ptr<GY_SRequest>;
            virtual int DecodePdu(const std::string &buffer) = 0;
        };

        class GY_CResponse{
        public:
            using ptr = std::shared_ptr<GY_CResponse>;
            using wptr = std::weak_ptr<GY_CResponse>;
            using uptr = std::unique_ptr<GY_CResponse>;
            virtual int DecodePdu(const std::string &buffer) = 0;
        };

        class GY_SResponse{
        public:
            using ptr = std::shared_ptr<GY_SResponse>;
            using wptr = std::weak_ptr<GY_SResponse>;
            using uptr = std::unique_ptr<GY_SResponse>;
            virtual std::string EncodePdu() = 0;
        };
        
        class GY_CRequest{
        public:
            using ptr = std::shared_ptr<GY_CRequest>;
            using wptr = std::weak_ptr<GY_CRequest>;
            using uptr = std::unique_ptr<GY_CRequest>;
            virtual std::string EncodePdu() = 0;
        };

        class GY_Request: public GY_SRequest, public GY_CRequest
        {
        public:
            using ptr = std::shared_ptr<GY_Request>;
            using wptr = std::weak_ptr<GY_Request>;
            using uptr = std::unique_ptr<GY_Request>;
            virtual int DecodePdu(const std::string &buffer) = 0;
            virtual std::string EncodePdu() = 0;
            virtual bool ParseIncomplete() final;
            virtual bool ParseIllegal() final;
            virtual bool ParseSuccess() final;
        protected:
            virtual void Incomplete() final;
            virtual void Illegal() final;
            virtual void Success() final;
        private:
            ProtoParseType m_parseStatus;
        };

        class GY_Response: public GY_SResponse, public GY_CResponse
        {
        public:
            using ptr = std::shared_ptr<GY_Response>;
            using wptr = std::weak_ptr<GY_Response>;
            using uptr = std::unique_ptr<GY_Response>;
            virtual int DecodePdu(const std::string &buffer) = 0;
            virtual std::string EncodePdu() = 0;
            virtual bool ParseIncomplete() final;
            virtual bool ParseIllegal() final;
            virtual bool ParseSuccess() final;
        protected:
            virtual void Incomplete() final;
            virtual void Illegal() final;
            virtual void Success() final;
        private:
            ProtoParseType m_parseStatus;
        };
        
    }
}

#endif