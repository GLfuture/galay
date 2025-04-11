#ifndef GALAY_INTERNAL_TCC
#define GALAY_INTERNAL_TCC

#include "Internal.hpp"

namespace galay::details
{

template<LoadBalancerType Type>
inline void SchedulerHolder<Type>::Initialize(std::vector<std::unique_ptr<typename Type::value_type>>&& schedulers)
{
    m_schedulers = std::forward<decltype(schedulers)>(schedulers);
}

template<LoadBalancerType Type>
inline void SchedulerHolder<Type>::StartAll()
{
    std::vector<typename Type::value_type*> scheduler_ptrs;
    int i = 0;
    scheduler_ptrs.reserve(m_schedulers.size());
    for( auto& scheduler : m_schedulers) {
        int try_count = MAX_START_SCHEDULERS_RETRY;
        scheduler_ptrs.push_back(scheduler.get());
        scheduler->Loop();
        while(!scheduler->IsRunning() && try_count-- >= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        if(try_count <= 0) {
            LogTrace("Start coroutine scheduler failed, index: {}", i);
            exit(-1);
        }
        ++i;
    }
    SchedulerLoadBalancer = std::make_unique<Type>(scheduler_ptrs);
}

template<LoadBalancerType Type>
inline void SchedulerHolder<Type>::StopAll()
{
    for( auto& scheduler : m_schedulers) {
        scheduler->Stop();
    }
}

template<LoadBalancerType Type>
inline SchedulerHolder<Type>* SchedulerHolder<Type>::GetInstance()
{
    if(!Instance) {
        Instance = std::make_unique<SchedulerHolder>();
    }
    return Instance.get();
}

template<LoadBalancerType Type>
inline typename Type::value_type* SchedulerHolder<Type>::GetScheduler()
{
    return SchedulerLoadBalancer.get()->Select();
}

template<LoadBalancerType Type>
inline typename Type::value_type* SchedulerHolder<Type>::GetScheduler(uint32_t index)
{
    return m_schedulers[index].get();
}

template <LoadBalancerType Type>
inline int SchedulerHolder<Type>::GetSchedulerSize()
{
    return m_schedulers.size();
}
}



#endif