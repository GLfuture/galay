#include "server.h"
#include "../common/signalhandler.h"
#include <csignal>
#include <spdlog/spdlog.h>

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
    this->m_threadPool = std::make_shared<common::GY_ThreadPool>();
}

galay::kernel::GY_TcpServerBuilderBase::ptr 
galay::kernel::GY_TcpServer::GetServerBuilder()
{
    return this->m_builder;
}

void 
galay::kernel::GY_TcpServer::Start()
{
    this->m_threadPool->Start(this->m_builder->GetNetThreadNum());
    for (int i = 0; i < m_builder->GetNetThreadNum(); ++i)
    {
        GY_IOScheduler::ptr scheduler = CreateScheduler();
        GY_TimerManager::ptr timerManager = CreateTimerManager();
        int timerfd = timerManager->GetTimerfd();
        scheduler->RegiserTimerManager(timerfd, timerManager);
        typename GY_Acceptor::ptr acceptor = CreateAcceptor(scheduler);
        scheduler->RegisterObjector(acceptor->GetListenFd(), acceptor);
        scheduler->AddEvent(acceptor->GetListenFd(), EventType::kEventRead | EventType::kEvnetEpollET);
        m_schedulers.push_back(scheduler);
        m_threadPool->AddTask([scheduler](){
            scheduler->Start();
        });
    }
    if(this->m_threadPool->WaitForAllDone()){
        spdlog::info("[{}:{}] [Program Exit Normally]",__FILE__,__LINE__);
    }else{
        spdlog::error("[{}:{}] [Program Exit Abnormally]",__FILE__,__LINE__);
    }
    
}

void 
galay::kernel::GY_TcpServer::Stop()
{
    if (!m_isStopped.load())
    {
        m_isStopped.store(true);
        spdlog::info("[{}:{}] [GY_TcpServer.Stop]",__FILE__,__LINE__);
        for (auto &scheduler : m_schedulers)
        {
            scheduler->Stop();
        }
        m_threadPool->Stop();
        spdlog::info("[{}:{}] [GY_TcpServer Exit Normally]",__FILE__,__LINE__);
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
        return std::make_shared<GY_EpollScheduler>(m_builder);
    case common::kSelectScheduler:
        return std::make_shared<GY_SelectScheduler>(m_builder);
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
}