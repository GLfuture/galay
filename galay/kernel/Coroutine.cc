#include "Coroutine.hpp"

namespace galay
{
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

details::CoroutineScheduler *RoutineCtx::GetScheduler()
{
    return m_scheduler.load();
}
}