#include "server.h"
#include "builder.h"
#include "poller.h"
#include "../common/coroutine.h"
#include "../common/signalhandler.h"
#include <csignal>
#include <spdlog/spdlog.h>

namespace galay::server
{
SIOManager::SIOManager(TcpServerBuilderBase::ptr builder)
{
    this->m_stop = false;
    switch(builder->GetSchedulerType())
    {
        case server::PollerType::kEpollPoller:
        {
            this->m_ioScheduler = std::make_shared<poller::EpollScheduler>(builder->GetMaxEventSize(), builder->GetScheWaitTime());
            break;
        }
        case server::PollerType::kSelectPoller:
        {
            this->m_ioScheduler = std::make_shared<poller::SelectScheduler>(builder->GetScheWaitTime());
            break;
        }
    }
}

void 
SIOManager::Start()
{
    m_ioScheduler->Start();
}

void 
SIOManager::Stop()
{
    if(!m_stop)
    {
        this->m_ioScheduler->Stop();
        this->m_stop = true;
    }
}

std::shared_ptr<poller::IOScheduler> 
SIOManager::GetIOScheduler()
{
    return m_ioScheduler;
}

SIOManager::~SIOManager()
{
    if(!m_stop)
    {
        this->Stop();
        m_stop = true;
    }
}

TcpServer::TcpServer()
{
    this->m_isStopped.store(false);
    galay::common::SignalFactory::GetInstance()->SetSignalHandler(SIGINT,[this](int signo){
        Stop();
    });
    galay::common::SignalFactory::GetInstance()->SetSignalHandler(SIGABRT,[this](int signo){
        Stop();
    });
    galay::common::SignalFactory::GetInstance()->SetSignalHandler(SIGPIPE,[this](int signo){
        Stop();
    });
    galay::common::SignalFactory::GetInstance()->SetSignalHandler(SIGALRM,[this](int signo){
        Stop();
    });
}

void 
TcpServer::SetServerBuilder(TcpServerBuilderBase::ptr builder)
{
    this->m_builder = builder;
    this->m_threadPool = std::make_shared<thread::ThreadPool>();
}

TcpServerBuilderBase::ptr 
TcpServer::GetServerBuilder()
{
    return this->m_builder;
}

void 
TcpServer::Start()
{
    this->m_threadPool->Start(this->m_builder->GetNetThreadNum());
    for (int i = 0; i < m_builder->GetNetThreadNum(); ++i)
    {
        SIOManager::ptr ioManager = std::make_shared<SIOManager>(this->m_builder);
        poller::TimerManager::ptr timerManager = std::make_shared<poller::TimerManager>();
        int timerfd = timerManager->GetTimerfd();
        ioManager->GetIOScheduler()->RegiserTimerManager(timerfd, timerManager);
        typename poller::TcpAcceptor::ptr acceptor = std::make_shared<poller::TcpAcceptor>(ioManager->GetIOScheduler(), this->m_builder->GetPort(), this->m_builder->GetBacklog(), this->m_builder->GetTypeName(kClassNameRequest), this->m_builder->GetSSLConfig());
        ioManager->GetIOScheduler()->RegisterObjector(acceptor->GetListenFd(), acceptor);
        ioManager->GetIOScheduler()->AddEvent(acceptor->GetListenFd(), poller::kEventEpollET | poller::kEventRead);
        m_ioManagers.push_back(ioManager);
        m_threadPool->AddTask([ioManager](){
            ioManager->Start();
        });
    }
    if(this->m_threadPool->WaitForAllDone()){
        spdlog::info("[{}:{}] [ThreadPool Exit Normally]",__FILE__,__LINE__);
    }else{
        spdlog::error("[{}:{}] [ThreadPool Exit Abnormally]",__FILE__,__LINE__);
    }
    if(coroutine::NetCoroutinePool::GetInstance()->IsDone() || coroutine::NetCoroutinePool::GetInstance()->WaitForAllDone()){
        spdlog::info("[{}:{}] [CoroutinePool Exit Normally]",__FILE__,__LINE__);
    }else{
        spdlog::error("[{}:{}] [CoroutinePool Exit Abnormally(timeout)]",__FILE__,__LINE__);
    }
    spdlog::info("[{}:{}] [Server Exit Normally]",__FILE__,__LINE__);
}

void 
TcpServer::Stop()
{
    if (!m_isStopped.load())
    {
        m_isStopped.store(true);
        spdlog::info("[{}:{}] [TcpServer.Stop]",__FILE__,__LINE__);
        for (auto &ioManager : m_ioManagers)
        {
            ioManager->Stop();
        }
        m_threadPool->Stop();
        coroutine::NetCoroutinePool::GetInstance()->Stop();
    }
}


SIOManager::wptr
TcpServer::GetManager(int indx)
{
    return this->m_ioManagers[indx];
}


TcpServer::~TcpServer()
{
    Stop();
}
}