#ifndef GALAY_INTERNAL_HPP
#define GALAY_INTERNAL_HPP

#include "galay/common/Base.h"
#include "galay/common/Error.h"
#include "Strategy.hpp"
#include "Log.h"

namespace galay
{
class AwaiterBase
{
};

}


namespace galay::details
{
class EventEngine;

extern std::string ToString(EventType type);

//must be alloc at heap
class Event
{
public:
    virtual std::string Name() = 0;
    virtual void HandleEvent(EventEngine* engine) = 0;
    virtual EventType GetEventType() = 0;
    virtual GHandle GetHandle() = 0;
    virtual bool SetEventEngine(EventEngine* engine) = 0;
    virtual EventEngine* BelongEngine() = 0;
    virtual ~Event() = default;
};


class CallbackEvent final : public Event
{
public:
    CallbackEvent(GHandle handle, EventType type, std::function<void(Event*, EventEngine*)> callback);
    void HandleEvent(EventEngine* engine) override;
    std::string Name() override { return "CallbackEvent"; }
    EventType GetEventType() override { return m_type; }
    GHandle GetHandle() override { return m_handle; }
    bool SetEventEngine(EventEngine* engine) override;
    EventEngine* BelongEngine() override;
    ~CallbackEvent() override;
private:
    EventType m_type;
    GHandle m_handle;
    std::atomic<EventEngine*> m_engine;
    std::function<void(Event*, EventEngine*)> m_callback;
};



template<typename T>
concept LoadBalancerType = requires(T t)
{
    typename T::value_type;
    requires std::is_constructible_v<T, const std::vector<typename T::value_type*>&>;
    requires std::is_same_v<decltype(t.Select()), typename T::value_type*>;
};

#define MAX_START_SCHEDULERS_RETRY   50

template<LoadBalancerType Type>
class SchedulerHolder
{
    static std::unique_ptr<Type> SchedulerLoadBalancer;
    static std::unique_ptr<SchedulerHolder> Instance;
public:

    //最少为1
    void Initialize(uint32_t scheduler_size, int timeout);
    void Initialize(std::vector<std::unique_ptr<typename Type::value_type>>&& schedulers);
    void Destroy();
    static SchedulerHolder* GetInstance();
    Type::value_type* GetScheduler();
    Type::value_type* GetScheduler(uint32_t index);
private:
    std::vector<std::unique_ptr<typename Type::value_type>> m_schedulers;
};

template<LoadBalancerType Type>
std::unique_ptr<Type> SchedulerHolder<Type>::SchedulerLoadBalancer;

template<LoadBalancerType Type>
std::unique_ptr<SchedulerHolder<Type>> SchedulerHolder<Type>::Instance;


}

#include "Internal.tcc"

#endif