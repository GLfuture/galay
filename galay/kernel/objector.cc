#include "objector.h"
#include "task.h"
#include <spdlog/spdlog.h>


galay::GY_Objector::GY_Objector()
{

}

galay::GY_Objector::~GY_Objector()
{

}

//timer
galay::Timer::Timer(uint64_t timerid, uint64_t during_time , uint32_t exec_times , ::std::function<::std::any()> &&func)
{
    this->m_timerid = timerid;
    this->m_exec_times = exec_times;
    this->m_userfunc = func;
    SetDuringTime(during_time);
}

uint64_t 
galay::Timer::GetCurrentTime()
{
    return ::std::chrono::duration_cast<::std::chrono::milliseconds>(::std::chrono::steady_clock::now().time_since_epoch()).count();
}

uint64_t 
galay::Timer::GetDuringTime()
{
    return this->m_during_time;
}

uint64_t 
galay::Timer::GetExpiredTime()
{
    return this->m_expired_time;
}

uint64_t 
galay::Timer::GetRemainTime()
{
    int64_t time = this->m_expired_time - Timer::GetCurrentTime();
    return time < 0 ? 0 : time;
}

uint64_t 
galay::Timer::GetTimerid()
{
    return this->m_timerid;
}

void 
galay::Timer::SetDuringTime(uint64_t during_time)
{
    this->m_during_time = during_time;
    this->m_expired_time = Timer::GetCurrentTime() + during_time;
}

uint32_t&
galay::Timer::GetRemainExecTimes()
{
    return this->m_exec_times;
}


void 
galay::Timer::Execute()
{
    this->m_is_finish = false;
    m_result = this->m_userfunc();
    if(this->m_exec_times == 0) this->m_is_finish = true;
}

::std::any 
galay::Timer::Result()
{
    return this->m_result;
}

// 取消任务
void galay::Timer::Cancle()
{
    this->m_cancle = true;
}

bool galay::Timer::IsCancled()
{
    return this->m_cancle;
}

// 是否已经完成
bool galay::Timer::IsFinish()
{
    return this->m_is_finish.load();
}

::std::atomic_uint64_t galay::GY_TimerManager::m_global_timerid = 0;

galay::GY_TimerManager::GY_TimerManager()
{
    this->m_timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
}

galay::Timer::ptr
galay::GY_TimerManager::AddTimer(uint64_t during, uint32_t exec_times, ::std::function<::std::any()> &&func)
{
    m_global_timerid.fetch_add(1, ::std::memory_order_acquire);
    ::std::unique_lock<::std::shared_mutex> lock(this->m_mtx);
    uint64_t timerid = m_global_timerid.load(::std::memory_order_acquire);
    if (timerid >= MAX_TIMERID)
    {
        m_global_timerid.store((timerid % MAX_TIMERID), ::std::memory_order_release);
    }
    auto timer = ::std::make_shared<Timer>(m_global_timerid.load(), during, exec_times, ::std::forward<::std::function<::std::any()> &&>(func));
    this->m_timers.push(timer);
    UpdateTimerfd();
    return timer;
}

void 
galay::GY_TimerManager::SetEventType(int event_type)
{
    
}

void 
galay::GY_TimerManager::UpdateTimerfd()
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

galay::Timer::ptr 
galay::GY_TimerManager::GetEaliestTimer()
{
    if (this->m_timers.empty())
        return nullptr;
    ::std::unique_lock<::std::shared_mutex> lock(this->m_mtx);
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
galay::GY_TimerManager::GetTimerfd(){ 
    return this->m_timerfd;  
}


void 
galay::GY_TimerManager::ExecuteTask()
{
    Timer::ptr timer = GetEaliestTimer();
    timer->Execute();
}


galay::GY_TimerManager::~GY_TimerManager()
{
    while (!m_timers.empty())
    {
        m_timers.pop();
    }
}

galay::GY_Acceptor::GY_Acceptor(::std::weak_ptr<GY_IOScheduler> scheduler)
{
    this->m_listentask = ::std::make_unique<GY_CreateConnTask>(scheduler);
}

int 
galay::GY_Acceptor::GetListenFd()
{
    return this->m_listentask->GetFd();
}

void 
galay::GY_Acceptor::SetEventType(int event_type)
{

}

void 
galay::GY_Acceptor::ExecuteTask() 
{
    this->m_listentask->Execute();
}

galay::GY_Acceptor::~GY_Acceptor()
{
    m_listentask.reset();
}


galay::GY_Receiver::GY_Receiver(int fd, ::std::weak_ptr<GY_IOScheduler> scheduler)
{
    this->m_recvTask = std::make_unique<GY_RecvTask>(fd, scheduler);
}

std::string& 
galay::GY_Receiver::GetRBuffer()
{
    return this->m_recvTask->GetRBuffer();
}

void 
galay::GY_Receiver::SetEventType(int event_type)
{

}

void 
galay::GY_Receiver::SetSSL(SSL* ssl)
{
    this->m_recvTask->SetSSL(ssl);
}

void 
galay::GY_Receiver::ExecuteTask()
{
    this->m_recvTask->RecvAll();
}

galay::GY_Sender::GY_Sender(int fd, ::std::weak_ptr<GY_IOScheduler> scheduler)
{
    this->m_sendTask = std::make_unique<GY_SendTask>(fd, scheduler);
}

void 
galay::GY_Sender::SetEventType(int event_type)
{
    
}

void 
galay::GY_Sender::AppendWBuffer(::std::string&& wbuffer)
{
    this->m_sendTask->AppendWBuffer(std::forward<::std::string&&>(wbuffer));
}

bool 
galay::GY_Sender::WBufferEmpty()
{
    return this->m_sendTask->Empty();
}

void 
galay::GY_Sender::ExecuteTask()
{
    this->m_sendTask->SendAll();
}

void 
galay::GY_Sender::SetSSL(SSL* ssl)
{
    this->m_sendTask->SetSSL(ssl);
}

galay::GY_Connector::GY_Connector(int fd, ::std::weak_ptr<GY_IOScheduler> scheduler)
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
galay::GY_Connector::SetEventType(int event_type)
{
    this->m_eventType = event_type;
}

::std::shared_ptr<galay::Timer> 
galay::GY_Connector::AddTimer(uint64_t during, uint32_t exec_times,::std::function<::std::any()> &&func)
{
    return this->m_scheduler.lock()->AddTimer(during,exec_times,::std::forward<::std::function<::std::any()>&&>(func));
}

void 
galay::GY_Connector::SetContext(::std::any&& context)
{
    this->m_context = std::forward<::std::any&&>(context);
}

std::any&&
galay::GY_Connector::GetContext()
{
    return std::move(this->m_context);
}

galay::protocol::GY_TcpRequest::ptr 
galay::GY_Connector::GetRequest()
{
    protocol::GY_TcpRequest::ptr request = m_requests.front();
    m_requests.pop();
    return request;
}

void 
galay::GY_Connector::PushResponse(galay::protocol::GY_TcpResponse::ptr response)
{
    m_responses.push(response);
    m_SendCoroutine.Resume();
}

void 
galay::GY_Connector::PushRequest(galay::protocol::GY_TcpRequest::ptr request)
{
    m_requests.push(request);
    if(this->m_WaitingForRequest){
        m_Maincoroutine.Resume();
    }
}

galay::protocol::GY_TcpResponse::ptr 
galay::GY_Connector::GetResponse()
{
    protocol::GY_TcpResponse::ptr response = m_responses.front();
    m_responses.pop();
    return response;
}

void 
galay::GY_Connector::ExecuteTask()
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
                m_scheduler.lock()->DelEvent(this->m_fd,EventType::GY_EVENT_READ|EventType::GY_EVENT_WRITE|EventType::GY_EVENT_ERROR);
                m_scheduler.lock()->DelObjector(this->m_fd);
                close(this->m_fd);
                return;
            }
        }else{
            spdlog::debug("[{}:{}] [SSLAccept(fd: {}) Success]", __FILE__, __LINE__, this->m_fd);
            m_scheduler.lock()->ModEvent(this->m_fd,EventType::GY_EVENT_WRITE,EventType::GY_EVENT_READ);
            m_is_ssl_accept = true;
            return;
        }
    }
    if(!this->m_controller) {
        this->m_controller = std::make_shared<GY_Controller>(shared_from_this());
    }
    if(m_eventType & GY_EVENT_READ){
        this->m_RecvCoroutine.Resume();
    }
    if(m_eventType & GY_EVENT_WRITE)
    {
        this->m_SendCoroutine.Resume();
    }
}

void 
galay::GY_Connector::SetSSLCtx(SSL_CTX* ctx)
{
    this->m_ssl = IOFunction::NetIOFunction::TcpFunction::SSLCreateObj(ctx,this->m_fd);
    this->m_receiver->SetSSL(this->m_ssl);
    this->m_sender->SetSSL(this->m_ssl);
}

galay::GY_TcpCoroutine<galay::CoroutineStatus> 
galay::GY_Connector::CoBusinessExec()
{
    co_await std::suspend_always{};
    while(1)
    {
        if(this->m_requests.empty()){
            this->m_WaitingForRequest = true;
            co_await std::suspend_always{};
        }
        this->m_WaitingForRequest = false;
        co_await DealUserFunc();
        if(this->m_controller->IsClosed() && this->m_sender->WBufferEmpty()){
            this->m_scheduler.lock()->DelEvent(this->m_fd,EventType::GY_EVENT_READ | EventType::GY_EVENT_WRITE | EventType::GY_EVENT_ERROR);
            this->m_scheduler.lock()->DelObjector(this->m_fd);
        }
    }
    co_return galay::GY_COROUTINE_FINISHED;
}

galay::GY_TcpCoroutine<galay::CoroutineStatus> 
galay::GY_Connector::CoReceiveExec()
{
    while(1)
    {
        co_await std::suspend_always{};
        m_receiver->ExecuteTask();
        while(1){
            if(!m_tempRequest) m_tempRequest = g_tcp_protocol_factory->CreateRequest();
            ProtoJudgeType type = m_tempRequest->DecodePdu(m_receiver->GetRBuffer());
            if(type == ProtoJudgeType::PROTOCOL_FINISHED){
                PushRequest(std::move(m_tempRequest));
                m_tempRequest = nullptr;
            }else if(type == ProtoJudgeType::PROTOCOL_INCOMPLETE)
            {
                break;
            }else{
                // To Do
                break;
            }
        }
    }
    co_return galay::CoroutineStatus::GY_COROUTINE_FINISHED;
}

galay::GY_TcpCoroutine<galay::CoroutineStatus> 
galay::GY_Connector::CoSendExec()
{
    while(1)
    {
        co_await std::suspend_always{};
        while (!m_responses.empty())
        {
            auto response = GetResponse();
            m_sender->AppendWBuffer(response->EncodePdu());
        }
        m_sender->ExecuteTask();
        if(m_sender->WBufferEmpty()){
            if(this->m_controller->IsClosed()){
                this->m_scheduler.lock()->DelEvent(this->m_fd,EventType::GY_EVENT_READ | EventType::GY_EVENT_WRITE | EventType::GY_EVENT_ERROR);
                this->m_scheduler.lock()->DelObjector(this->m_fd);
            }else {
                this->m_scheduler.lock()->DelEvent(this->m_fd,EventType::GY_EVENT_WRITE);
            }         
        }else{
            this->m_scheduler.lock()->AddEvent(this->m_fd,EventType::GY_EVENT_WRITE);
        }
    }
    co_return galay::CoroutineStatus::GY_COROUTINE_FINISHED;
}

galay::CommonAwaiter 
galay::GY_Connector::DealUserFunc()
{
    this->m_UserCoroutine = this->m_scheduler.lock()->UserFunction(this->m_controller);
    if(this->m_UserCoroutine.IsCoroutine() && this->m_UserCoroutine.GetStatus() == CoroutineStatus::GY_COROUTINE_SUSPEND){
        this->m_UserCoroutine.SetFatherCoroutine(this->m_UserCoroutine);
        return CommonAwaiter(true,-1);
    }
    return CommonAwaiter(false,0);
}

galay::GY_Connector::~GY_Connector()
{
    if(this->m_ssl){
        IOFunction::NetIOFunction::TcpFunction::SSLDestory(this->m_ssl);
        this->m_ssl = nullptr;
    }
}