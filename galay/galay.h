#ifndef GALAY_H
#define GALAY_H

//common
#include "common/base.h"
#include "common/signalhandler.h"
#include "common/threadpool.h"

//kernel
#include "kernel/client.h"
#include "kernel/server.h"
#include "kernel/factory.h"
#include "kernel/router.h"

//protocol
#include "protocol/http.h"
#include "protocol/smtp.h"

//util
#include "util/parser.h"
#include "util/fileiofunction.h"
#include "util/ratelimiter.h"



namespace galay
{
    class GY_TcpCoreBase
    {
    public:
        using ptr = std::shared_ptr<GY_TcpCoreBase>;
        using wptr = std::weak_ptr<GY_TcpCoreBase>;
        using uptr = std::unique_ptr<GY_TcpCoreBase>;
        using GY_NetCoroutine = galay::common::GY_NetCoroutine<galay::common::CoroutineStatus>;
        using GY_Controller = galay::kernel::GY_Controller;
        
        virtual GY_NetCoroutine CoreBusiness(GY_Controller::wptr ctrl) = 0;
    };

    class GY_HttpCoreBase
    {
    public:
        using ptr = std::shared_ptr<GY_HttpCoreBase>;
        using wptr = std::weak_ptr<GY_HttpCoreBase>;
        using uptr = std::unique_ptr<GY_HttpCoreBase>;
        using GY_NetCoroutine = galay::common::GY_NetCoroutine<galay::common::CoroutineStatus>;
        using GY_HttpController = galay::kernel::GY_HttpController;

        virtual GY_NetCoroutine CoreBusiness(GY_HttpController::wptr ctrl) = 0;
    };
}

#endif