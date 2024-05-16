#include "server.h"
#include <spdlog/spdlog.h>
void galay::GY_TcpServer::Start(GY_TcpServerBuilderBase::ptr builder)
{
    for (int i = 0; i < builder->GetThreadNum(); ++i)
    {
        GY_IOScheduler::ptr scheduler = CreateScheduler(builder);
        GY_TimerManager::ptr timerManager = CreateTimerManager();
        int timerfd = timerManager->GetTimerfd();
        scheduler->RegiserTimerManager(timerfd, timerManager);
        typename GY_Acceptor::ptr acceptor = CreateAcceptor(scheduler);
        scheduler->RegisterObjector(acceptor->GetListenFd(), acceptor);
        scheduler->AddEvent(acceptor->GetListenFd(), EventType::GY_EVENT_READ | EventType::GY_EVENT_EPOLLET);
        m_schedulers.push_back(scheduler);
        m_threads.push_back(::std::make_shared<::std::thread>(&GY_IOScheduler::Start, scheduler));
    }
    WaitForThreads();
}

void galay::GY_TcpServer::Stop()
{
    for (auto &scheduler : m_schedulers)
    {
        scheduler->Stop();
    }
}

galay::GY_TimerManager::ptr
galay::GY_TcpServer::CreateTimerManager()
{
    return ::std::make_shared<GY_TimerManager>();
}

galay::GY_IOScheduler::ptr
galay::GY_TcpServer::CreateScheduler(GY_TcpServerBuilderBase::ptr builder)
{
    switch (builder->GetSchedulerType())
    {
    case GY_TcpServerBuilderBase::SchedulerType::EPOLL_SCHEDULER:
        return ::std::make_shared<GY_EpollScheduler>(builder);
    case GY_TcpServerBuilderBase::SchedulerType::SELECT_SCHEDULER:
        return ::std::make_shared<GY_SelectScheduler>(builder);
    }
    return nullptr;
}

galay::GY_IOScheduler::wptr
galay::GY_TcpServer::GetScheduler(int indx)
{
    return this->m_schedulers[indx];
}


galay::GY_Acceptor::ptr
galay::GY_TcpServer::CreateAcceptor( GY_IOScheduler::ptr scheduler)
{
    return ::std::make_shared<GY_Acceptor>(scheduler);
}

void galay::GY_TcpServer::WaitForThreads()
{
    for (auto &th : m_threads)
    {
        if (th->joinable())
            th->join();
    }
}

galay::GY_TcpServer::~GY_TcpServer()
{
    for (int i = 0; i < this->m_schedulers.size(); ++i)
    {
        this->m_schedulers[i]->Stop();
    }
    for (int i = 0; i < this->m_threads.size(); ++i)
    {
        WaitForThreads();
    }
    this->m_schedulers.clear();
    this->m_threads.clear();
}