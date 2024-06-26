#include "objector.h"
#include "task.h"
#include "waitgroup.h"
#include "../common/reflection.h"
#include <spdlog/spdlog.h>


galay::kernel::GY_Objector::GY_Objector()
{

}

galay::kernel::GY_Objector::~GY_Objector()
{

}

//timer
galay::kernel::Timer::Timer(uint64_t timerid, uint64_t during_time , uint32_t exec_times , std::function<std::any()> &&func)
{
    this->m_timerid = timerid;
    this->m_exec_times = exec_times;
    this->m_userfunc = func;
    SetDuringTime(during_time);
}

uint64_t 
galay::kernel::Timer::GetCurrentTime()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

uint64_t 
galay::kernel::Timer::GetDuringTime()
{
    return this->m_during_time;
}

uint64_t 
galay::kernel::Timer::GetExpiredTime()
{
    return this->m_expired_time;
}

uint64_t 
galay::kernel::Timer::GetRemainTime()
{
    int64_t time = this->m_expired_time - Timer::GetCurrentTime();
    return time < 0 ? 0 : time;
}

uint64_t 
galay::kernel::Timer::GetTimerid()
{
    return this->m_timerid;
}

void 
galay::kernel::Timer::SetDuringTime(uint64_t during_time)
{
    this->m_during_time = during_time;
    this->m_expired_time = Timer::GetCurrentTime() + during_time;
}

uint32_t&
galay::kernel::Timer::GetRemainExecTimes()
{
    return this->m_exec_times;
}


void 
galay::kernel::Timer::Execute()
{
    this->m_is_finish = false;
    m_result = this->m_userfunc();
    if(this->m_exec_times == 0) this->m_is_finish = true;
}

std::any 
galay::kernel::Timer::Result()
{
    return this->m_result;
}

// 取消任务
void galay::kernel::Timer::Cancle()
{
    this->m_cancle = true;
}

bool galay::kernel::Timer::IsCancled()
{
    return this->m_cancle;
}

// 是否已经完成
bool galay::kernel::Timer::IsFinish()
{
    return this->m_is_finish.load();
}

std::atomic_uint64_t galay::kernel::GY_TimerManager::m_global_timerid = 0;

galay::kernel::GY_TimerManager::GY_TimerManager()
{
    this->m_timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
}

galay::kernel::Timer::ptr
galay::kernel::GY_TimerManager::AddTimer(uint64_t during, uint32_t exec_times, std::function<std::any()> &&func)
{
    m_global_timerid.fetch_add(1, std::memory_order_acquire);
    std::unique_lock<std::shared_mutex> lock(this->m_mtx);
    uint64_t timerid = m_global_timerid.load(std::memory_order_acquire);
    if (timerid >= MAX_TIMERID)
    {
        m_global_timerid.store((timerid % MAX_TIMERID), std::memory_order_release);
    }
    auto timer = std::make_shared<Timer>(m_global_timerid.load(), during, exec_times, std::forward<std::function<std::any()> &&>(func));
    this->m_timers.push(timer);
    UpdateTimerfd();
    return timer;
}

void 
galay::kernel::GY_TimerManager::SetEventType(int event_type)
{
    
}

void 
galay::kernel::GY_TimerManager::UpdateTimerfd()
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

galay::kernel::Timer::ptr 
galay::kernel::GY_TimerManager::GetEaliestTimer()
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
galay::kernel::GY_TimerManager::GetTimerfd(){ 
    return this->m_timerfd;  
}


void 
galay::kernel::GY_TimerManager::ExecuteTask()
{
    Timer::ptr timer = GetEaliestTimer();
    timer->Execute();
}


galay::kernel::GY_TimerManager::~GY_TimerManager()
{
    while (!m_timers.empty())
    {
        m_timers.pop();
    }
}

galay::kernel::GY_Acceptor::GY_Acceptor(std::weak_ptr<GY_IOScheduler> scheduler)
{
    this->m_listentask = std::make_unique<GY_CreateConnTask>(scheduler);
}

int 
galay::kernel::GY_Acceptor::GetListenFd()
{
    return this->m_listentask->GetFd();
}

void 
galay::kernel::GY_Acceptor::SetEventType(int event_type)
{

}

void 
galay::kernel::GY_Acceptor::ExecuteTask() 
{
    this->m_listentask->Execute();
}

galay::kernel::GY_Acceptor::~GY_Acceptor()
{
    m_listentask.reset();
}


galay::kernel::GY_Receiver::GY_Receiver(int fd, std::weak_ptr<GY_IOScheduler> scheduler)
{
    this->m_recvTask = std::make_unique<GY_RecvTask>(fd, scheduler);
}

std::string& 
galay::kernel::GY_Receiver::GetRBuffer()
{
    return this->m_recvTask->GetRBuffer();
}

void 
galay::kernel::GY_Receiver::SetEventType(int event_type)
{

}

void 
galay::kernel::GY_Receiver::SetSSL(SSL* ssl)
{
    this->m_recvTask->SetSSL(ssl);
}

void 
galay::kernel::GY_Receiver::ExecuteTask()
{
    this->m_recvTask->RecvAll();
}

galay::kernel::GY_Sender::GY_Sender(int fd, std::weak_ptr<GY_IOScheduler> scheduler)
{
    this->m_sendTask = std::make_unique<GY_SendTask>(fd, scheduler);
}

void 
galay::kernel::GY_Sender::SetEventType(int event_type)
{
    
}

void 
galay::kernel::GY_Sender::AppendWBuffer(std::string&& wbuffer)
{
    this->m_sendTask->AppendWBuffer(std::forward<std::string&&>(wbuffer));
}

bool 
galay::kernel::GY_Sender::WBufferEmpty()
{
    return this->m_sendTask->Empty();
}

void 
galay::kernel::GY_Sender::ExecuteTask()
{
    this->m_sendTask->SendAll();
}

void 
galay::kernel::GY_Sender::SetSSL(SSL* ssl)
{
    this->m_sendTask->SetSSL(ssl);
}

galay::kernel::GY_Connector::GY_Connector(int fd, std::weak_ptr<GY_IOScheduler> scheduler)
{
    this->m_fd = fd;
    this->m_is_ssl_accept = false;
    this->m_ssl = nullptr;
    this->m_scheduler = scheduler;
    this->m_receiver = std::make_unique<GY_Receiver>(fd, scheduler);
    this->m_sender = std::make_unique<GY_Sender>(fd, scheduler);
    this->m_WaitingForRequest = true;
    this->m_controller = nullptr;
    this->m_Maincoroutine = CoBusinessExec();
    this->m_RecvCoroutine = CoReceiveExec();
    this->m_SendCoroutine = CoSendExec();
}

void 
galay::kernel::GY_Connector::SetEventType(int event_type)
{
    this->m_eventType = event_type;
}

std::shared_ptr<galay::kernel::Timer> 
galay::kernel::GY_Connector::AddTimer(uint64_t during, uint32_t exec_times,std::function<std::any()> &&func)
{
    return this->m_scheduler.lock()->AddTimer(during,exec_times,std::forward<std::function<std::any()>&&>(func));
}

void 
galay::kernel::GY_Connector::SetContext(std::any&& context)
{
    this->m_context = std::forward<std::any&&>(context);
}

std::any&&
galay::kernel::GY_Connector::GetContext()
{
    return std::move(this->m_context);
}

galay::protocol::GY_Request::ptr 
galay::kernel::GY_Connector::GetRequest()
{
    protocol::GY_Request::ptr request = m_requests.front();
    m_requests.pop();
    return request;
}

void 
galay::kernel::GY_Connector::PushResponse(std::string&& response)
{
    m_responses.push(response);
    m_SendCoroutine.Resume();
}

void 
galay::kernel::GY_Connector::PushRequest(galay::protocol::GY_Request::ptr request)
{
    m_requests.push(request);
    if(this->m_WaitingForRequest){
        m_Maincoroutine.Resume();
    }
}

void 
galay::kernel::GY_Connector::ExecuteTask()
{
    if(m_scheduler.lock()->GetTcpServerBuilder().lock()->GetIsSSL() && !m_is_ssl_accept){
        int ret = IOFunction::NetIOFunction::TcpFunction::SSLAccept(this->m_ssl);
        if (ret <= 0)
        {
            int ssl_err = SSL_get_error(this->m_ssl, ret);
            if (ssl_err == SSL_ERROR_WANT_READ || ssl_err == SSL_ERROR_WANT_WRITE || ssl_err == SSL_ERROR_WANT_ACCEPT)
            {
                return;
            }
            else
            {
                char msg[256];
                ERR_error_string(ssl_err, msg);
                spdlog::error("[{}:{}] [socket(fd: {}) SSL_Accept error: '{}']", __FILE__, __LINE__, this->m_fd, msg);
                IOFunction::NetIOFunction::TcpFunction::SSLDestory(this->m_ssl);
                this->m_ssl = nullptr;
                spdlog::error("[{}:{}] [socket(fd: {}) close]", __FILE__, __LINE__, this->m_fd);
                m_scheduler.lock()->DelEvent(this->m_fd,EventType::kEventRead|EventType::kEventWrite|EventType::kEventError);
                m_scheduler.lock()->DelObjector(this->m_fd);
                close(this->m_fd);
                return;
            }
        }else{
            spdlog::debug("[{}:{}] [SSLAccept(fd: {}) Success]", __FILE__, __LINE__, this->m_fd);
            m_scheduler.lock()->ModEvent(this->m_fd,EventType::kEventWrite,EventType::kEventRead);
            m_is_ssl_accept = true;
            return;
        }
    }
    if(!this->m_controller) {
        this->m_controller = std::make_shared<GY_Controller>(shared_from_this());
    }
    if(m_eventType & kEventRead){
        this->m_RecvCoroutine.Resume();
    }
    if(m_eventType & kEventWrite)
    {
        this->m_SendCoroutine.Resume();
    }
}

void 
galay::kernel::GY_Connector::SetSSLCtx(SSL_CTX* ctx)
{
    this->m_ssl = IOFunction::NetIOFunction::TcpFunction::SSLCreateObj(ctx,this->m_fd);
    this->m_receiver->SetSSL(this->m_ssl);
    this->m_sender->SetSSL(this->m_ssl);
}

galay::common::GY_TcpCoroutine<galay::common::CoroutineStatus> 
galay::kernel::GY_Connector::CoBusinessExec()
{
    co_await std::suspend_always{};
    while(1)
    {
        if(this->m_requests.empty()){
            this->m_WaitingForRequest = true;
            co_await std::suspend_always{};
        }
        this->m_WaitingForRequest = false;
        WaitGroup group;
        group.Add(1);
        this->m_controller->SetWaitGroup(&group);
        this->m_UserCoroutine = this->m_scheduler.lock()->UserFunction(this->m_controller);
        co_await group.Wait();
        if(this->m_controller->IsClosed() && this->m_sender->WBufferEmpty()){
            this->m_scheduler.lock()->DelEvent(this->m_fd,EventType::kEventRead | EventType::kEventWrite | EventType::kEventError);
            this->m_scheduler.lock()->DelObjector(this->m_fd);
        }
    }
    co_return common::CoroutineStatus::kCoroutineFinished;
}

galay::common::GY_TcpCoroutine<galay::common::CoroutineStatus> 
galay::kernel::GY_Connector::CoReceiveExec()
{
    while(1)
    {
        co_await std::suspend_always{};
        m_receiver->ExecuteTask();
        while(1){
            if(!m_tempRequest) m_tempRequest = common::GY_RequestFactory<>::GetInstance()->Create(this->m_scheduler.lock()->GetTcpServerBuilder().lock()->GetTypeName(common::kClassNameRequest));
            if(!m_tempRequest) {
                spdlog::error("[{}:{}] [CoReceiveExec Create RequestObj Fail, TypeName: {}]",__FILE__,__LINE__,this->m_scheduler.lock()->GetTcpServerBuilder().lock()->GetTypeName(common::kClassNameRequest));
                break;
            }
            std::string& buffer = m_receiver->GetRBuffer();
            if(buffer.length() == 0) break;
            common::ProtoJudgeType type = m_tempRequest->DecodePdu(buffer);
            if(type == common::ProtoJudgeType::kProtoFinished){
                PushRequest(std::move(m_tempRequest));
                m_tempRequest = nullptr;
            }else if(type == common::ProtoJudgeType::kProtoIncomplete)
            {
                break;
            }else{
                // To Do
                break;
            }
        }
    }
    co_return common::CoroutineStatus::kCoroutineFinished;
}

galay::common::GY_TcpCoroutine<galay::common::CoroutineStatus> 
galay::kernel::GY_Connector::CoSendExec()
{
    while(1)
    {
        co_await std::suspend_always{};
        while (!m_responses.empty())
        {
            m_sender->AppendWBuffer(std::move(m_responses.front()));
            m_responses.pop();
        }
        m_sender->ExecuteTask();
        if(m_sender->WBufferEmpty()){
            if(this->m_controller->IsClosed()){
                this->m_scheduler.lock()->DelEvent(this->m_fd,EventType::kEventRead | EventType::kEventWrite | EventType::kEventError);
                this->m_scheduler.lock()->DelObjector(this->m_fd);
            }else {
                this->m_scheduler.lock()->ModEvent(this->m_fd,EventType::kEventWrite , EventType::kEventRead);
            }         
        }else{
            this->m_scheduler.lock()->ModEvent(this->m_fd,EventType::kEventRead, EventType::kEventRead | EventType::kEventWrite | EventType::kEventError);
        }
    }
    co_return common::CoroutineStatus::kCoroutineFinished;
}

galay::kernel::GY_Connector::~GY_Connector()
{
    if(this->m_ssl){
        IOFunction::NetIOFunction::TcpFunction::SSLDestory(this->m_ssl);
        this->m_ssl = nullptr;
    }
}
