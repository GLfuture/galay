#include "server.h"
#include "../common/signalhandler.h"
#include <csignal>
#include <spdlog/spdlog.h>

galay::kernel::GY_ThreadCond::GY_ThreadCond(uint16_t threadNum)
{
    this->m_threadNum.store(threadNum);
}

void 
galay::kernel::GY_ThreadCond::DecreaseThread()
{
    this->m_threadNum.fetch_sub(1);
    if (this->m_threadNum.load() == 0)
    {
        m_thCond.notify_one();
    }
}

void 
galay::kernel::GY_ThreadCond::WaitForThreads()
{
    std::unique_lock lock(this->m_thMutex);
    m_thCond.wait(lock,[this](){
        return m_threadNum.load() == 0;
    });
}


galay::kernel::GY_TcpServer::GY_TcpServer()
{
    this->m_isStopped.store(false);
    galay::common::GY_SignalFactory::GetInstance()->SetSignalHandler(SIGINT,[this](int signo){
        Stop();
    });
    galay::common::GY_SignalFactory::GetInstance()->SetSignalHandler(SIGABRT,[this](int signo){
        Stop();
    });
    galay::common::GY_SignalFactory::GetInstance()->SetSignalHandler(SIGPIPE,[this](int signo){
        Stop();
    });
    galay::common::GY_SignalFactory::GetInstance()->SetSignalHandler(SIGALRM,[this](int signo){
        Stop();
    });
}

void 
galay::kernel::GY_TcpServer::SetServerBuilder(GY_TcpServerBuilderBase::ptr builder)
{
    this->m_builder = builder;
    this->m_threadCond = std::make_shared<GY_ThreadCond>(builder->GetThreadNum());
}

galay::kernel::GY_TcpServerBuilderBase::ptr 
galay::kernel::GY_TcpServer::GetServerBuilder()
{
    return this->m_builder;
}

void 
galay::kernel::GY_TcpServer::Start()
{
    for (int i = 0; i < m_builder->GetThreadNum(); ++i)
    {
        GY_IOScheduler::ptr scheduler = CreateScheduler();
        GY_TimerManager::ptr timerManager = CreateTimerManager();
        int timerfd = timerManager->GetTimerfd();
        scheduler->RegiserTimerManager(timerfd, timerManager);
        typename GY_Acceptor::ptr acceptor = CreateAcceptor(scheduler);
        scheduler->RegisterObjector(acceptor->GetListenFd(), acceptor);
        scheduler->AddEvent(acceptor->GetListenFd(), EventType::kEventRead | EventType::kEvnetEpollET);
        m_schedulers.push_back(scheduler);
        m_threads.push_back(std::make_shared<std::thread>(&GY_IOScheduler::Start, scheduler));
        m_threads.back()->detach();
    }
    std::mutex mtx;
    std::unique_lock lock(mtx);
    m_exitCond.wait(lock);
    spdlog::info("[{}:{}] [Program Exit Normally]",__FILE__,__LINE__);
}

void 
galay::kernel::GY_TcpServer::Stop()
{
    if (!m_isStopped.load())
    {
        spdlog::info("[{}:{}] [GY_TcpServer.Stop]",__FILE__,__LINE__);
        for (auto &scheduler : m_schedulers)
        {
            scheduler->Stop();
        }
        m_threadCond->WaitForThreads();
        spdlog::info("[{}:{}] [GY_TcpServer Exit Normally]",__FILE__,__LINE__);
        m_exitCond.notify_one();
    }
}


galay::kernel::GY_TimerManager::ptr
galay::kernel::GY_TcpServer::CreateTimerManager()
{
    return std::make_shared<GY_TimerManager>();
}

galay::kernel::GY_IOScheduler::ptr
galay::kernel::GY_TcpServer::CreateScheduler()
{
    switch (m_builder->GetSchedulerType())
    {
    case common::kEpollScheduler:
        return std::make_shared<GY_EpollScheduler>(m_builder,m_threadCond);
    case common::kSelectScheduler:
        return std::make_shared<GY_SelectScheduler>(m_builder,m_threadCond);
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

galay::kernel::GY_TcpServer::~GY_TcpServer()
{
    Stop();
    m_schedulers.clear();
    m_threads.clear();
}