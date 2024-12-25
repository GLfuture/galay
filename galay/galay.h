#ifndef GALAY_H
#define GALAY_H

#include "helper/HttpHelper.h"

//common
#include "common/Base.h"
#include "common/SignalHandler.h"
#include "common/Reflection.h"

//kernel
#include "kernel/Coroutine.hpp"
#include "kernel/Async.h"
#include "kernel/Event.hpp"
#include "kernel/Scheduler.h"
#include "kernel/Session.hpp"
#include "kernel/Server.hpp"
#include "kernel/ExternApi.hpp"
#include "kernel/Log.h"
#include "kernel/Time.h"

//protocol
#include "protocol/Http.h"
#include "protocol/Smtp.h"
#include "protocol/Dns.h"

//utils
#include "utils/Parser.h"
#include "utils/Io.h"
#include "utils/String.h"
#include "utils/TypeName.h"
#include "utils/Random.h"
#include "utils/RateLimiter.h"
#include "utils/Tree.h"
#include "utils/Thread.h"

namespace galay 
{
#ifndef GALAY_VERSION
#define GALAY_VERSION "0.0.2"
#endif

#define HTTPGET protocol::http::HttpMethod::Http_Method_Get
#define HTTPPOST protocol::http::HttpMethod::Http_Method_Post
#define HTTPDELETE protocol::http::HttpMethod::Http_Method_Delete
#define HTTPPUT protocol::http::HttpMethod::Http_Method_Put
#define HTTPPATCH protocol::http::HttpMethod::Http_Method_Patch
#define HTTPHEAD protocol::http::HttpMethod::Http_Method_Head
#define HTTPOPTIONS protocol::http::HttpMethod::Http_Method_Options
#define HTTPTRACE protocol::http::HttpMethod::Http_Method_Trace
#define HTTPCONNECT protocol::http::HttpMethod::Http_Method_Connect


}

#define GALAY_APP_MAIN(custom_code) \
{ \
    galay::InitializeGalayEnv(DEFAULT_COROUTINE_SCHEDULER_CONF, DEFAULT_NETWORK_SCHEDULER_CONF, DEFAULT_TIMER_SCHEDULER_CONF ); \
    custom_code; \
    galay::DestroyGalayEnv(); \
}



#endif