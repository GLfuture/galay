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
#include "kernel/Time.hpp"

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

#define DEFAULT_COROUTINE_SCHEDULER_THREAD_NUM          4
#define DEFAULT_EVENT_SCHEDULER_THREAD_NUM              4
#define DEFAULT_SESSION_SCHEDULER_CONF                  {1, -1}

struct CoroutineSchedulerConf
{
    int m_thread_num = DEFAULT_COROUTINE_SCHEDULER_THREAD_NUM;
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