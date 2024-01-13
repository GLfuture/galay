#include "timer.h"

uint32_t galay::Timer::m_global_timerid = 0;

galay::Timer_Manager::Timer_Manager(std::weak_ptr<IO_Scheduler> scheduler)
{
    this->m_timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
    this->m_scheduler = scheduler;
}