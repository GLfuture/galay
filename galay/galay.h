#ifndef GALAY_H
#define GALAY_H

//common

//kernel
#include "kernel/client.h"
#include "kernel/server.h"
#include "kernel/factory.h"
#include "kernel/router.h"

//protocol
#include "protocol/http1_1.h"
#include "protocol/smtp.h"

//other
#include "kernel/waitgroup.h"

//util
#include "util/parser.h"
#include "util/fileiofunction.h"
#include "util/ratelimiter.h"


namespace galay
{
    class GY_CoreBase
    {
    public:
        using ptr = std::shared_ptr<GY_CoreBase>;
        using wptr = std::weak_ptr<GY_CoreBase>;
        using uptr = std::unique_ptr<GY_CoreBase>;
    };
}

#endif