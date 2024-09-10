#ifndef GALAY_H
#define GALAY_H

//common
#include "common/base.h"
#include "common/signalhandler.h"
#include "common/threadpool.h"
#include "common/coroutine.h"
#include "common/reflection.h"
#include "common/waitgroup.h"
#include "common/result.h"

//kernel
#include "kernel/client.h"
#include "kernel/server.h"
#include "kernel/factory.h"
#include "kernel/router.h"
#include "kernel/builder.h"
#include "kernel/controller.h"
#include "kernel/poller.h"

//protocol
#include "protocol/http.h"
#include "protocol/smtp.h"
#include "protocol/dns.h"

//util
#include "util/parser.h"
#include "util/io.h"
#include "util/stringtools.h"
#include "util/typename.h"
#include "util/random.h"
#include "util/ratelimiter.h"
#include "util/tree.h"



namespace galay
{
    class TcpCoreBase
    {
    public:
        using ptr = std::shared_ptr<TcpCoreBase>;
        using wptr = std::weak_ptr<TcpCoreBase>;
        using uptr = std::unique_ptr<TcpCoreBase>;
        using NetCoroutine = coroutine::NetCoroutine;
        using Controller = server::Controller;
        
        virtual NetCoroutine CoreBusiness(Controller::ptr ctrl) = 0;
    };

    class HttpCoreBase
    {
    public:
        using ptr = std::shared_ptr<HttpCoreBase>;
        using wptr = std::weak_ptr<HttpCoreBase>;
        using uptr = std::unique_ptr<HttpCoreBase>;
        using NetCoroutine = coroutine::NetCoroutine;
        using HttpController = server::HttpController;

        virtual NetCoroutine CoreBusiness(HttpController::ptr ctrl) = 0;
    };
}

#endif