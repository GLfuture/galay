#ifndef GALAY_HTTP2_H
#define GALAY_HTTP2_H

#include "http1_1.h"

#define MAX_FRAME_SIZE 16384

namespace galay
{
    namespace protocol{
        namespace http{
            class Http2_0_Protocol : public Http1_1_Protocol
            {
            public:
            };
        }
    }
    
}



#endif