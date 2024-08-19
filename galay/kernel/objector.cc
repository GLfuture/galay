#include "objector.h"
#include "task.h"
#include "../common/coroutine.h"
#include "../common/waitgroup.h"
#include "../common/reflection.h"
#include "../util/random.h"
#include "result.h"
#include "server.h"
#include "scheduler.h"
#include "builder.h"
#include <spdlog/spdlog.h>

namespace galay::objector
{
GY_Objector::GY_Objector()
{

}

GY_Objector::~GY_Objector()
{

}

//timer
Timer::Timer(uint64_t timerid, uint64_t during_time , uint32_t exec_times , std::function<void(Timer::ptr)> &&func)
{
    this->m_timerid = timerid;
    this->m_execTimes = exec_times;
    this->m_rightHandle = func;
    SetDuringTime(during_time);
}

uint64_t 
Timer::GetCurrentTime()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

uint64_t 
Timer::GetDuringTime()
{
    return this->m_duringTime;
}

uint64_t 
Timer::GetExpiredTime()
{
    return this->m_expiredTime;
}

uint64_t 
Timer::GetRemainTime()
{
    int64_t time = this->m_expiredTime - Timer::GetCurrentTime();
    return time < 0 ? 0 : time;
}

uint64_t 
Timer::GetTimerId()
{
    return this->m_timerid;
}

void 
Timer::SetDuringTime(uint64_t duringTime)
{
    this->m_duringTime = duringTime;
    this->m_expiredTime = Timer::GetCurrentTime() + duringTime;
}

uint32_t&
Timer::GetRemainExecTimes()
{
    return this->m_execTimes;
}


void 
Timer::Execute()
{
    this->m_success = false;
    this->m_rightHandle(shared_from_this());
    if(this->m_execTimes == 0) this->m_success = true;
}


// 取消任务
void Timer::Cancle()
{
    this->m_cancle = true;
}

bool Timer::IsCancled()
{
    return this->m_cancle;
}

// 是否已经完成
bool Timer::Success()
{
    return this->m_success.load();
}

std::atomic_uint64_t GY_TimerManager::m_global_timerid = 0;

GY_TimerManager::GY_TimerManager()
{
    this->m_timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
    this->m_readCallback += [this](){
        Timer::ptr timer = GetEaliestTimer();
        timer->Execute();
    };
}

Timer::ptr
GY_TimerManager::AddTimer(uint64_t during, uint32_t exec_times, std::function<void(Timer::ptr)> &&func)
{
    m_global_timerid.fetch_add(1, std::memory_order_acquire);
    std::unique_lock<std::shared_mutex> lock(this->m_mtx);
    uint64_t timerid = m_global_timerid.load(std::memory_order_acquire);
    if (timerid >= MAX_TIMERID)
    {
        m_global_timerid.store((timerid % MAX_TIMERID), std::memory_order_release);
    }
    auto timer = std::make_shared<Timer>(m_global_timerid.load(), during, exec_times, std::forward<std::function<void(Timer::ptr)>>(func));
    this->m_timers.push(timer);
    UpdateTimerfd();
    return timer;
}

void 
GY_TimerManager::UpdateTimerfd()
{
    struct timespec abstime;
    if (m_timers.empty())
    {
        abstime.tv_sec = 0;
        abstime.tv_nsec = 0;
    }
    else
    {
        int64_t time = m_timers.top()->GetRemainTime();
        if (time != 0)
        {
            abstime.tv_sec = time / 1000;
            abstime.tv_nsec = (time % 1000) * 1000000;
        }
        else
        {
            abstime.tv_sec = 0;
            abstime.tv_nsec = 1;
        }
    }
    struct itimerspec its = {
        .it_interval = {},
        .it_value = abstime};
    timerfd_settime(this->m_timerfd, 0, &its, nullptr);
}

Timer::ptr 
GY_TimerManager::GetEaliestTimer()
{
    if (this->m_timers.empty())
        return nullptr;
    std::unique_lock<std::shared_mutex> lock(this->m_mtx);
    auto timer = this->m_timers.top();
    this->m_timers.pop();
    if (--timer->GetRemainExecTimes() > 0)
    {
        timer->SetDuringTime(timer->GetDuringTime());
        this->m_timers.push(timer);
    }
    UpdateTimerfd();
    return timer;
}

int 
GY_TimerManager::GetTimerfd(){ 
    return this->m_timerfd;  
}

Callback& 
GY_TimerManager::OnRead()
{
    return m_readCallback;
}

Callback& 
GY_TimerManager::OnWrite()
{
    return m_sendCallback;
}


GY_TimerManager::~GY_TimerManager()
{
    while (!m_timers.empty())
    {
        m_timers.pop();
    }
}

GY_TcpAcceptor::GY_TcpAcceptor(std::weak_ptr<server::GY_SIOManager> manager)
{
    if(manager.lock()->GetTcpServerBuilder().lock()->GetIsSSL()){
        this->m_listentask = std::make_unique<task::GY_TcpCreateSSLConnTask>(manager);
    }else{
        this->m_listentask = std::make_unique<task::GY_TcpCreateConnTask>(manager);
    }
    this->m_readCallback += [this](){
        this->m_listentask->Execute();
    };
}

int 
GY_TcpAcceptor::GetListenFd()
{
    return this->m_listentask->GetFd();
}
Callback& 
GY_TcpAcceptor::OnRead()
{
    return m_readCallback;
}

Callback& 
GY_TcpAcceptor::OnWrite()
{
    return m_sendCallback;
}

GY_TcpAcceptor::~GY_TcpAcceptor()
{
    m_listentask.reset();
}

GY_TcpConnector::GY_TcpConnector(int fd, SSL* ssl, std::weak_ptr<server::GY_SIOManager> ioManager)
{
    this->m_fd = fd;
    this->m_ssl = ssl;
    this->m_isClosed = false;
    this->m_ioManager = ioManager;
    this->m_recvTask = std::make_unique<task::GY_TcpRecvTask>(fd, ssl, ioManager.lock()->GetIOScheduler());
    this->m_sendTask = std::make_unique<task::GY_TcpSendTask>(fd, ssl, ioManager.lock()->GetIOScheduler());
    this->m_controller = nullptr;
    this->m_readCallback += [this](){
        RealRecv();
    };
}

void 
GY_TcpConnector::Close()
{
    if(!m_isClosed) 
    {
        spdlog::info("[{}:{}] [Close Connection(fd: {})]", __FILE__, __LINE__, this->m_fd);
        this->m_ioManager.lock()->GetIOScheduler()->DelEvent(this->m_fd, poller::kEventRead | poller::kEventWrite | poller::kEventError);
        this->m_ioManager.lock()->GetIOScheduler()->DelObjector(this->m_fd);
        close(this->m_fd);
        this->m_isClosed = true;
    }
}

std::shared_ptr<Timer> 
GY_TcpConnector::AddTimer(uint64_t during, uint32_t exec_times,std::function<void(std::shared_ptr<Timer>)> &&func)
{
    return this->m_ioManager.lock()->GetIOScheduler()->AddTimer(during,exec_times,std::forward<std::function<void(std::shared_ptr<Timer>)>>(func));
}

galay::protocol::GY_Request::ptr 
GY_TcpConnector::GetRequest()
{
    if(m_requests.empty()) return nullptr;
    return m_requests.front();
}

void 
GY_TcpConnector::PopRequest()
{
    if(!m_requests.empty()) m_requests.pop();
}

bool 
GY_TcpConnector::HasRequest()
{
    return !m_requests.empty();
}

Callback& 
GY_TcpConnector::OnRead()
{
    return m_readCallback;
}

Callback&
GY_TcpConnector::OnWrite()
{
    return m_sendCallback;
}

result::NetResult::ptr 
GY_TcpConnector::Send(std::string&& response)
{
    result::NetResultInner::ptr result = std::make_shared<result::NetResultInner>();
    this->m_sendTask->AppendWBuffer(std::forward<std::string>(response));
    this->m_sendTask->SendAll();
    if(this->m_sendTask->Empty()){
        result->SetSuccess(true);
        goto end;
    }else{
        result->AddTaskNum(1);
        result->SetErrorMsg("Waiting");
        m_sendCallback += [result,this](){
            RealSend(result);
        };
        this->m_ioManager.lock()->GetIOScheduler()->ModEvent(this->m_fd, poller::kEventRead, poller::kEventWrite | poller::kEventError | poller::kEventEpollET);
    }
end:
    return result;
}


//先send再加epoll 
void 
GY_TcpConnector::RealSend(std::shared_ptr<result::NetResultInner> result)
{
    this->m_sendTask->SendAll();
    if(this->m_sendTask->Empty())
    {
        this->m_ioManager.lock()->GetIOScheduler()->ModEvent(this->m_fd, poller::kEventRead, poller::kEventWrite | poller::kEventError | poller::kEventEpollET);
        if(result) {
            result->SetSuccess(true);
            result->Done();
        }
    }
}

void 
GY_TcpConnector::RealRecv()
{
    if(!this->m_controller) {
        this->m_controller = std::make_shared<server::GY_Controller>(shared_from_this());
    }
    m_recvTask->RecvAll();
    while(true)
    {
        if(!m_tempRequest) m_tempRequest = common::GY_RequestFactory<>::GetInstance()->Create(this->m_ioManager.lock()->GetTcpServerBuilder().lock()->GetTypeName(common::kClassNameRequest));
        if(!m_tempRequest) 
        {
            spdlog::error("[{}:{}] [CoReceiveExec Create RequestObj Fail, TypeName: {}]",__FILE__,__LINE__, this->m_ioManager.lock()->GetTcpServerBuilder().lock()->GetTypeName(common::kClassNameRequest));
            return;
        }
        std::string& buffer = m_recvTask->GetRBuffer();
        if(!m_recvTask->Success()){
            if(buffer.empty()) return;
        }
        spdlog::debug("[{}:{}] [CoReceiveExec Recv Buffer: {}]",__FILE__,__LINE__, buffer);
        int eLen = m_tempRequest->DecodePdu(buffer);
        if(m_tempRequest->ParseSuccess())
        {
            buffer.erase(0, eLen);
            m_requests.push(m_tempRequest);
            m_tempRequest = common::GY_RequestFactory<>::GetInstance()->Create(this->m_ioManager.lock()->GetTcpServerBuilder().lock()->GetTypeName(common::kClassNameRequest));
        }
        else if(m_tempRequest->ParseIncomplete())
        {
            break;
        }
        else if(m_tempRequest->ParseIllegal())
        {
            this->m_controller->SetError(m_tempRequest->Error());
            m_tempRequest = common::GY_RequestFactory<>::GetInstance()->Create(this->m_ioManager.lock()->GetTcpServerBuilder().lock()->GetTypeName(common::kClassNameRequest));
            this->m_ioManager.lock()->WrongHandle(this->m_controller);
            buffer.clear();
            return;
        }
    }
    if(m_requests.empty()) return;
    this->m_ioManager.lock()->RightHandle(this->m_controller);
}

GY_TcpConnector::~GY_TcpConnector()
{
    if(this->m_ssl){
        IOFunction::NetIOFunction::TcpFunction::SSLDestory(this->m_ssl);
        this->m_ssl = nullptr;
    }
}

Callback& 
GY_ClientExcutor::OnRead()
{
    return this->m_readCallback;
}

Callback& 
GY_ClientExcutor::OnWrite()
{
    return this->m_sendCallback;
}
}