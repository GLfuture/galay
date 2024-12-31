#ifndef GALAY_H
#define GALAY_H

#include "helper/HttpHelper.h"

//common
#include "common/Base.h"
#include "common/SignalHandler.h"
#include "common/Reflection.h"

//kernel
#include "kernel/Coroutine.hpp"
#include "kernel/Async.hpp"
#include "kernel/Server.hpp"
#include "kernel/ExternApi.hpp"

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
#include "utils/ArgsParse.hpp"

namespace galay 
{
#ifndef GALAY_VERSION
#define GALAY_VERSION "0.0.2"
#endif

#define HTTPGET http::HttpMethod::Http_Method_Get
#define HTTPPOST http::HttpMethod::Http_Method_Post
#define HTTPDELETE http::HttpMethod::Http_Method_Delete
#define HTTPPUT http::HttpMethod::Http_Method_Put
#define HTTPPATCH http::HttpMethod::Http_Method_Patch
#define HTTPHEAD http::HttpMethod::Http_Method_Head
#define HTTPOPTIONS http::HttpMethod::Http_Method_Options
#define HTTPTRACE http::HttpMethod::Http_Method_Trace
#define HTTPCONNECT http::HttpMethod::Http_Method_Connect


}

#define GALAY_APP_MAIN(custom_code) \
{ \
    galay::InitializeGalayEnv(DEFAULT_COROUTINE_SCHEDULER_CONF, DEFAULT_NETWORK_SCHEDULER_CONF, DEFAULT_TIMER_SCHEDULER_CONF ); \
    custom_code; \
    galay::DestroyGalayEnv(); \
}



#endif