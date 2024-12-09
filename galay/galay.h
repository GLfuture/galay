#ifndef GALAY_H
#define GALAY_H

#include "helper/HttpHelper.h"

//common
#include "common/Base.h"
#include "common/SignalHandler.h"
#include "common/Reflection.h"

//kernel
#include "kernel/Coroutine.h"
#include "kernel/Async.h"
#include "kernel/Event.h"
#include "kernel/Scheduler.h"
#include "kernel/Connection.h"
#include "kernel/Server.h"
#include "kernel/ExternApi.h"
#include "kernel/Log.h"
#include "kernel/Time.h"

//protocol
#include "protocol/Http.h"
#include "protocol/Smtp.h"
#include "protocol/Dns.h"

//util
#include "util/Parser.h"
#include "util/Io.h"
#include "util/String.h"
#include "util/TypeName.h"
#include "util/Random.h"
#include "util/RateLimiter.h"
#include "util/Tree.h"
#include "util/Thread.h"
#include "util/ThreadSefe.hpp"

#ifndef GALAY_VERSION
#define GALAY_VERSION "0.0.1"
#endif

#define GALAY_APP_MAIN(custom_code) \
{ \
    galay::InitializeGalayEnv(DEFAULT_COROUTINE_SCHEDULER_CONF, DEFAULT_NETWORK_SCHEDULER_CONF, DEFAULT_TIMER_SCHEDULER_CONF ); \
    custom_code; \
    galay::DestroyGalayEnv(); \
}



#endif