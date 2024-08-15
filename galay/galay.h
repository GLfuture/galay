#ifndef GALAY_H
#define GALAY_H

//common
#include "common/base.h"
#include "common/signalhandler.h"
#include "common/threadpool.h"
#include "common/coroutine.h"

//kernel
#include "kernel/client.h"
#include "kernel/server.h"
#include "kernel/factory.h"
#include "kernel/router.h"
#include "kernel/builder.h"
#include "kernel/controller.h"
#include "kernel/scheduler.h"
#include "kernel/result.h"

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
        using GY_NetCoroutine = coroutine::GY_NetCoroutine;
        using GY_Controller = kernel::GY_Controller;
        
        virtual GY_NetCoroutine CoreBusiness(GY_Controller::ptr ctrl) = 0;
    };

    class GY_HttpCoreBase
    {
    public:
        using ptr = std::shared_ptr<GY_HttpCoreBase>;
        using wptr = std::weak_ptr<GY_HttpCoreBase>;
        using uptr = std::unique_ptr<GY_HttpCoreBase>;
        using GY_NetCoroutine = coroutine::GY_NetCoroutine;
        using GY_HttpController = kernel::GY_HttpController;

        virtual GY_NetCoroutine CoreBusiness(GY_HttpController::ptr ctrl) = 0;
    };
}

#endif