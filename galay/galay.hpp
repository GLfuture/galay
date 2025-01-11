#ifndef GALAY_HPP
#define GALAY_HPP

#include "helper/HttpHelper.h"

//common
#include "common/Base.h"
#include "common/SignalHandler.h"
#include "common/Reflection.h"

//kernel
#include "kernel/Coroutine.hpp"
#include "kernel/Async.hpp"
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
#include "utils/System.h"
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

#define DEFAULT_COROUTINE_SCHEDULER_CONF                {4, -1}
#define DEFAULT_NETWORK_SCHEDULER_CONF                  {4, -1}
#define DEFAULT_TIMER_SCHEDULER_CONF                    {1, -1}
#define DEFAULT_SESSION_SCHEDULER_CONF                  {1, -1}

struct GalayEnvConf 
{
    using ptr = std::shared_ptr<GalayEnvConf>;
    static GalayEnvConf::ptr Create();
    std::pair<uint32_t, int> m_coroutineSchedulerConf = DEFAULT_COROUTINE_SCHEDULER_CONF;
    std::pair<uint32_t, int> m_eventSchedulerConf = DEFAULT_NETWORK_SCHEDULER_CONF;
    std::pair<uint32_t, int> m_timerSchedulerConf = DEFAULT_TIMER_SCHEDULER_CONF;
    std::pair<uint32_t, int> m_sessionSchedulerConf = DEFAULT_SESSION_SCHEDULER_CONF; //各个中间件的调度器配置
};

class GalayEnv
{
public:
    GalayEnv(GalayEnvConf conf);
    virtual ~GalayEnv();
};


}


#include "galay.inl"

#endif