#ifndef GALAY_HPP
#define GALAY_HPP

//common
#include "common/Base.h"
#include "common/SignalHandler.h"
#include "common/Reflection.h"

//kernel
#include "kernel/Server.hpp"
#include "kernel/Client.hpp"
#include "kernel/ExternApi.hpp"

//protocol
#include "protocol/http/HttpServer.hpp"
#include "protocol/http/HttpClient.hpp"
#include "protocol/smtp/Smtp.h"
#include "protocol/dns/Dns.h"
#include "protocol/http2/Http2_0.hpp"

//utils
#include "utils/Parser.h"
#include "utils/System.h"
#include "utils/String.h"
#include "utils/TypeName.h"
#include "utils/Random.h"
#include "utils/RateLimiter.h"
#include "utils/CricuitBreaker.hpp"
#include "utils/Tree.h"
#include "utils/Thread.h"
#include "utils/App.hpp"
#include "utils/Mvcc.hpp"
#include "utils/Pool.hpp"
#include "utils/Distributed.hpp"

namespace galay 
{
#define _CONCAT(a, b) a##b
#define _MAKE_DEFER_(line) DeferClass _CONCAT(defer_placeholder, line) = [&]()

#undef DEFER
#define DEFER _MAKE_DEFER_(__LINE__)

#define DEFAULT_COROUTINE_SCHEDULER_THREAD_NUM          4
#define DEFAULT_EVENT_SCHEDULER_THREAD_NUM              4
#define DEFAULT_SESSION_SCHEDULER_CONF                  {1, -1}

struct CoroutineSchedulerConf
{
    int m_thread_num = DEFAULT_COROUTINE_SCHEDULER_THREAD_NUM;
};

enum IOInterfaceReturnType
{
    eReturnOtherFailed = -3,
    eReturnTimeOutFailed = -2,
    eReturnNonBlocking = -1,
    eReturnDisConnect = 0,
};

enum EventSchedulerTimerManagerType {
    kEventSchedulerTimerManagerTypeHeap = 0,
    kEventSchedulerTimerManagerTypeRbTree,
    kEventSchedulerTimerManagerTypeTimeWheel
};



struct EventSchedulerConf
{
    int m_thread_num = DEFAULT_EVENT_SCHEDULER_THREAD_NUM;
    EventSchedulerTimerManagerType m_time_manager_type = kEventSchedulerTimerManagerTypeHeap;
};

struct GalayEnvConf 
{
    using ptr = std::shared_ptr<GalayEnvConf>;
    static GalayEnvConf::ptr Create();
    CoroutineSchedulerConf  m_coroutineSchedulerConf;
    EventSchedulerConf      m_eventSchedulerConf;
};

class GalayEnv
{
public:
    GalayEnv(GalayEnvConf conf);
    virtual ~GalayEnv();
};


}


inline galay::GalayEnvConf::ptr galay::GalayEnvConf::Create()
{
    return std::make_shared<GalayEnvConf>();
}

inline galay::GalayEnv::GalayEnv(GalayEnvConf conf)
{
    std::vector<std::unique_ptr<details::CoroutineScheduler>> co_schedulers;
    co_schedulers.reserve(conf.m_coroutineSchedulerConf.m_thread_num);
    std::vector<std::unique_ptr<details::EventScheduler>> event_schedulers;
    event_schedulers.reserve(conf.m_eventSchedulerConf.m_thread_num);
    for(int i = 0; i < conf.m_coroutineSchedulerConf.m_thread_num; ++i ) {
        co_schedulers.emplace_back(std::make_unique<details::CoroutineScheduler>());
    }
    for(int i = 0; i < conf.m_eventSchedulerConf.m_thread_num; ++i ) {
        event_schedulers.emplace_back(std::make_unique<details::EventScheduler>());
        event_schedulers[i]->InitTimeEvent(static_cast<TimerManagerType>(conf.m_eventSchedulerConf.m_time_manager_type));
    }
    InitializeGalayEnv(std::move(co_schedulers), std::move(event_schedulers));
    StartGalayEnv();
}

inline galay::GalayEnv::~GalayEnv()
{
    DestroyGalayEnv();
}


#endif