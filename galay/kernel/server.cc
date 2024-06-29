#include "server.h"
#include <spdlog/spdlog.h>

void 
galay::kernel::GY_TcpServer::SetServerBuilder(GY_TcpServerBuilderBase::ptr builder)
{
    this->m_builder = builder;
}

void 
galay::kernel::GY_TcpServer::Start()
{
    for (int i = 0; i < m_builder->GetThreadNum(); ++i)
    {
        GY_IOScheduler::ptr scheduler = CreateScheduler(m_builder);
        GY_TimerManager::ptr timerManager = CreateTimerManager();
        int timerfd = timerManager->GetTimerfd();
        scheduler->RegiserTimerManager(timerfd, timerManager);
        typename GY_Acceptor::ptr acceptor = CreateAcceptor(scheduler);
        scheduler->RegisterObjector(acceptor->GetListenFd(), acceptor);
        scheduler->AddEvent(acceptor->GetListenFd(), EventType::kEventRead | EventType::kEvnetEpollET);
        m_schedulers.push_back(scheduler);
        m_threads.push_back(std::make_shared<std::thread>(&GY_IOScheduler::Start, scheduler));
    }
    WaitForThreads();
}

void 
galay::kernel::GY_TcpServer::Stop()
{
    for (auto &scheduler : m_schedulers)
    {
        scheduler->Stop();
    }
}

galay::kernel::GY_TimerManager::ptr
galay::kernel::GY_TcpServer::CreateTimerManager()
{
    return std::make_shared<GY_TimerManager>();
}

galay::kernel::GY_IOScheduler::ptr
galay::kernel::GY_TcpServer::CreateScheduler(GY_TcpServerBuilderBase::ptr builder)
{
    switch (builder->GetSchedulerType())
    {
    case common::kEpollScheduler:
        return std::make_shared<GY_EpollScheduler>(builder);
    case common::kSelectScheduler:
        return std::make_shared<GY_SelectScheduler>(builder);
    }
    return nullptr;
}

galay::kernel::GY_IOScheduler::wptr
galay::kernel::GY_TcpServer::GetScheduler(int indx)
{
    return this->m_schedulers[indx];
}


galay::kernel::GY_Acceptor::ptr
galay::kernel::GY_TcpServer::CreateAcceptor( GY_IOScheduler::ptr scheduler)
{
    return std::make_shared<GY_Acceptor>(scheduler);
}

void galay::kernel::GY_TcpServer::WaitForThreads()
{
    for (auto &th : m_threads)
    {
        if (th->joinable())
            th->join();
    }
}

galay::kernel::GY_TcpServer::~GY_TcpServer()
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