#include "Coroutine.hpp"
#include "Time.h"

namespace galay
{
RoutineCtx::ptr RoutineCtx::Create()
{
    return std::make_shared<RoutineCtx>();
}

RoutineCtx::RoutineCtx()
    :m_scheduler(CoroutineSchedulerHolder::GetInstance()->GetScheduler())
{
}

RoutineCtx::RoutineCtx(details::CoroutineScheduler *scheduler)
    :m_scheduler(scheduler)
{
}

RoutineCtx::RoutineCtx(const RoutineCtx &ctx)
{
    this->m_scheduler.store(ctx.m_scheduler);
}

RoutineCtx::RoutineCtx(RoutineCtx &&ctx)
{
    this->m_scheduler.store(std::move(ctx.m_scheduler));
    ctx.m_scheduler.store(nullptr);
}

Timer::ptr RoutineCtx::WithTimeout(int64_t timeout, std::function<void()>&& callback)
{
    auto co = m_co;
    return TimerSchedulerHolder::GetInstance()->GetScheduler()->AddTimer(timeout, [callback, co](details::TimeEvent::wptr event, Timer::ptr timer) {
        if(co.expired()) return;
        callback();
        co.lock()->BelongScheduler()->ToDestroyCoroutine(co);
    });
}

details::CoroutineScheduler *RoutineCtx::GetScheduler()
{
    return m_scheduler.load();
}

}