#include "server.h"
#include "builder.h"
#include "objector.h"
#include "scheduler.h"
#include "../common/coroutine.h"
#include "../common/signalhandler.h"
#include <csignal>
#include <spdlog/spdlog.h>

namespace galay::server
{
GY_SIOManager::GY_SIOManager(GY_TcpServerBuilderBase::ptr builder)
{
    this->m_stop = false;
    this->m_builder = builder;
    switch(builder->GetSchedulerType())
    {
        case common::SchedulerType::kEpollScheduler:
        {
            this->m_ioScheduler = std::make_shared<poller::GY_EpollScheduler>(builder->GetMaxEventSize(), builder->GetScheWaitTime());
            break;
        }
        case common::SchedulerType::kSelectScheduler:
        {
            this->m_ioScheduler = std::make_shared<poller::GY_SelectScheduler>(builder->GetScheWaitTime());
            break;
        }
    }
    this->m_rightHandle = builder->GetRightHandle();
    this->m_wrongHandle = builder->GetWrongHandle();
    coroutine::GY_NetCoroutinePool::GetInstance()->Start();
}

void 
GY_SIOManager::Start()
{
    m_ioScheduler->Start();
}

void 
GY_SIOManager::Stop()
{
    if(!m_stop)
    {
        this->m_ioScheduler->Stop();
        this->m_stop = true;
    }
}

std::shared_ptr<poller::GY_IOScheduler> 
GY_SIOManager::GetIOScheduler()
{
    return m_ioScheduler;
}

GY_TcpServerBuilderBase::wptr 
GY_SIOManager::GetTcpServerBuilder()
{
    return this->m_builder;
}

void
GY_SIOManager::RightHandle(std::shared_ptr<GY_Controller> controller)
{
    if(!this->m_rightHandle) {
        spdlog::error("[{}:{}] [RightHandle] [Error: RightHandle is nullptr]", __FILE__, __LINE__);
        return;
    }
    return this->m_rightHandle(controller);
}


void 
GY_SIOManager::WrongHandle(std::shared_ptr<GY_Controller> controller)
{
    if(!this->m_wrongHandle) {
        spdlog::error("[{}:{}] [WrongHandle] [Error: WrongHandle is nullptr]", __FILE__, __LINE__);
        return;
    }
    return this->m_wrongHandle(controller);
}

GY_SIOManager::~GY_SIOManager()
{
    if(!m_stop)
    {
        this->Stop();
        m_stop = true;
    }
}

GY_TcpServer::GY_TcpServer()
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
GY_TcpServer::SetServerBuilder(GY_TcpServerBuilderBase::ptr builder)
{
    this->m_builder = builder;
    this->m_threadPool = std::make_shared<thread::GY_ThreadPool>();
}

GY_TcpServerBuilderBase::ptr 
GY_TcpServer::GetServerBuilder()
{
    return this->m_builder;
}

void 
GY_TcpServer::Start()
{
    coroutine::GY_NetCoroutinePool::GetInstance()->Start();
    this->m_threadPool->Start(this->m_builder->GetNetThreadNum());
    for (int i = 0; i < m_builder->GetNetThreadNum(); ++i)
    {
        GY_SIOManager::ptr ioManager = std::make_shared<GY_SIOManager>(this->m_builder);
        objector::GY_TimerManager::ptr timerManager = CreateTimerManager();
        int timerfd = timerManager->GetTimerfd();
        ioManager->GetIOScheduler()->RegiserTimerManager(timerfd, timerManager);
        typename objector::GY_TcpAcceptor::ptr acceptor = CreateAcceptor(ioManager);
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
    if(coroutine::GY_NetCoroutinePool::GetInstance()->IsDone() || coroutine::GY_NetCoroutinePool::GetInstance()->WaitForAllDone()){
        spdlog::info("[{}:{}] [CoroutinePool Exit Normally]",__FILE__,__LINE__);
    }else{
        spdlog::error("[{}:{}] [CoroutinePool Exit Abnormally(timeout)]",__FILE__,__LINE__);
    }
    spdlog::info("[{}:{}] [Server Exit Normally]",__FILE__,__LINE__);
}

void 
GY_TcpServer::Stop()
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


std::shared_ptr<objector::GY_TimerManager>
GY_TcpServer::CreateTimerManager()
{
    return std::make_shared<objector::GY_TimerManager>();
}

std::shared_ptr<objector::GY_TcpAcceptor>
GY_TcpServer::CreateAcceptor( GY_SIOManager::ptr ioManager)
{
    return std::make_shared<objector::GY_TcpAcceptor>(ioManager);
}

GY_SIOManager::wptr
GY_TcpServer::GetManager(int indx)
{
    return this->m_ioManagers[indx];
}


GY_TcpServer::~GY_TcpServer()
{
    Stop();
}
}