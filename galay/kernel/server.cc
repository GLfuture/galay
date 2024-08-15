#include "server.h"
#include "builder.h"
#include "objector.h"
#include "scheduler.h"
#include "../common/coroutine.h"
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
    coroutine::GY_NetCoroutinePool::GetInstance()->Start();
    this->m_threadPool->Start(this->m_builder->GetNetThreadNum());
    for (int i = 0; i < m_builder->GetNetThreadNum(); ++i)
    {
        GY_SIOManager::ptr ioManager = std::make_shared<GY_SIOManager>(this->m_builder);
        GY_TimerManager::ptr timerManager = CreateTimerManager();
        int timerfd = timerManager->GetTimerfd();
        ioManager->GetIOScheduler()->RegiserTimerManager(timerfd, timerManager);
        typename GY_TcpAcceptor::ptr acceptor = CreateAcceptor(ioManager);
        ioManager->GetIOScheduler()->RegisterObjector(acceptor->GetListenFd(), acceptor);
        ioManager->GetIOScheduler()->AddEvent(acceptor->GetListenFd(), EventType::kEventRead | EventType::kEventEpollET);
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
    if(coroutine::GY_NetCoroutinePool::GetInstance()->IsDone() || coroutine::GY_NetCoroutinePool::GetInstance()->WaitForAllDone()){
        spdlog::info("[{}:{}] [CoroutinePool Exit Normally]",__FILE__,__LINE__);
    }else{
        spdlog::error("[{}:{}] [CoroutinePool Exit Abnormally(timeout)]",__FILE__,__LINE__);
    }
    spdlog::info("[{}:{}] [Server Exit Normally]",__FILE__,__LINE__);
}

void 
galay::kernel::GY_TcpServer::Stop()
{
    if (!m_isStopped.load())
    {
        m_isStopped.store(true);
        spdlog::info("[{}:{}] [GY_TcpServer.Stop]",__FILE__,__LINE__);
        for (auto &ioManager : m_ioManagers)
        {
            ioManager->Stop();
        }
        m_threadPool->Stop();
        coroutine::GY_NetCoroutinePool::GetInstance()->Stop();
    }
}


galay::kernel::GY_TimerManager::ptr
galay::kernel::GY_TcpServer::CreateTimerManager()
{
    return std::make_shared<GY_TimerManager>();
}

galay::kernel::GY_SIOManager::wptr
galay::kernel::GY_TcpServer::GetManager(int indx)
{
    return this->m_ioManagers[indx];
}


galay::kernel::GY_TcpAcceptor::ptr
galay::kernel::GY_TcpServer::CreateAcceptor( GY_SIOManager::ptr ioManager)
{
    return std::make_shared<GY_TcpAcceptor>(ioManager);
}

galay::kernel::GY_TcpServer::~GY_TcpServer()
{
    Stop();
}