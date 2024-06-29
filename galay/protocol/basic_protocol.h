#ifndef GALAY_BASIC_PROTOCOL_H
#define GALAY_BASIC_PROTOCOL_H

#include <string>
#include <memory>
#include "../common/base.h"
#include "../common/reflection.h"

namespace galay
{
    namespace protocol
    {
        class GY_Request{
        public:
            using ptr = std::shared_ptr<GY_Request>;
            using wptr = std::weak_ptr<GY_Request>;
            using uptr = std::unique_ptr<GY_Request>;
            virtual galay::common::ProtoJudgeType DecodePdu(std::string &buffer) = 0;
            virtual void Clear() = 0;
        };

        class GY_Response{
        public:
            using ptr = std::shared_ptr<GY_Response>;
            using wptr = std::weak_ptr<GY_Response>;
            using uptr = std::unique_ptr<GY_Response>;
            virtual std::string EncodePdu() = 0;
            virtual void Clear() = 0;
            
        };
        
    }
}

#endif