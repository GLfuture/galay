#include "objector.h"
#include "task.h"
#include "../common/waitgroup.h"
#include "../common/reflection.h"
#include "../util/random.h"
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
    if(scheduler.lock()->GetTcpServerBuilder().lock()->GetIsSSL()){
        this->m_listentask = std::make_unique<GY_CreateSSLConnTask>(scheduler);
    }else{
        this->m_listentask = std::make_unique<GY_CreateConnTask>(scheduler);
    }
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


galay::kernel::GY_Receiver::GY_Receiver(int fd, SSL* ssl,std::weak_ptr<GY_IOScheduler> scheduler)
{
    this->m_recvTask = std::make_unique<GY_RecvTask>(fd,ssl,scheduler);
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
galay::kernel::GY_Receiver::ExecuteTask()
{
    this->m_recvTask->RecvAll();
}

galay::kernel::GY_Sender::GY_Sender(int fd, SSL* ssl, std::weak_ptr<GY_IOScheduler> scheduler)
{
    this->m_sendTask = std::make_unique<GY_SendTask>(fd, ssl, scheduler);
    
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

galay::kernel::GY_Connector::GY_Connector(int fd, SSL* ssl, std::weak_ptr<GY_IOScheduler> scheduler)
{
    this->m_fd = fd;
    this->m_is_ssl_accept = false;
    this->m_ssl = ssl;
    this->m_scheduler = scheduler;
    this->m_receiver = std::make_unique<GY_Receiver>(fd, ssl, scheduler);
    this->m_sender = std::make_unique<GY_Sender>(fd, ssl, scheduler);
    this->m_exit = std::make_shared<bool>(false);
    this->m_controller = nullptr;

    auto Co = CoBusinessExec();
    this->m_MainCoId = Co.GetCoId();
    if(Co.IsCoroutine()){
        this->m_scheduler.lock()->GetCoroutinePool()->AddCoroutine(this->m_MainCoId,std::move(Co));
    }
    Co = CoReceiveExec();
    this->m_RecvCoId = Co.GetCoId();
    if(Co.IsCoroutine()){
        this->m_scheduler.lock()->GetCoroutinePool()->AddCoroutine(this->m_RecvCoId,std::move(Co));
    }
    Co = CoSendExec();
    this->m_SendCoId = Co.GetCoId();
    if(Co.IsCoroutine()){
        this->m_scheduler.lock()->GetCoroutinePool()->AddCoroutine(this->m_SendCoId,std::move(Co));
    }
    spdlog::debug("[{}:{}] [GY_Connector::GY_Connector() Business CoId = {}, Recv CoId = {}, Send CoId = {}]", __FILE__, __LINE__, this->m_MainCoId, this->m_RecvCoId, this->m_SendCoId);
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

std::weak_ptr<galay::common::GY_NetCoroutinePool> 
galay::kernel::GY_Connector::GetCoPool()
{
    return this->m_scheduler.lock()->GetCoroutinePool();
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
    this->m_scheduler.lock()->GetCoroutinePool()->Resume(this->m_SendCoId, true);
}

void 
galay::kernel::GY_Connector::PushRequest(galay::protocol::GY_Request::ptr request)
{
    m_requests.push(request);
    this->m_scheduler.lock()->GetCoroutinePool()->Resume(this->m_MainCoId, true);
}

void 
galay::kernel::GY_Connector::ExecuteTask()
{
    if(!this->m_controller) {
        this->m_controller = std::make_shared<GY_Controller>(shared_from_this());
    }
    if(m_eventType & kEventRead){
        m_receiver->ExecuteTask();
        this->m_scheduler.lock()->GetCoroutinePool()->Resume(this->m_RecvCoId, true);
    }
    if(m_eventType & kEventWrite)
    {
        this->m_scheduler.lock()->GetCoroutinePool()->Resume(this->m_SendCoId, true);
    }
}

galay::common::GY_NetCoroutine<galay::common::CoroutineStatus> 
galay::kernel::GY_Connector::CoBusinessExec()
{
    while(1)
    {
        if(this->m_requests.empty()){
            co_await std::suspend_always{};
        }
        WaitGroup group(this->m_scheduler.lock()->GetCoroutinePool());
        group.Add(1);
        this->m_controller->SetWaitGroup(&group);
        auto UserCoroutine = this->m_scheduler.lock()->UserFunction(this->m_controller);
        if(UserCoroutine.IsCoroutine()){
            this->m_UserCoId = UserCoroutine.GetCoId();
            this->m_scheduler.lock()->GetCoroutinePool()->AddCoroutine(UserCoroutine.GetCoId(),std::move(UserCoroutine));
        }
        co_await group.Wait();
        if(this->m_controller->IsClosed() && this->m_sender->WBufferEmpty()){
            this->m_scheduler.lock()->DelEvent(this->m_fd,EventType::kEventRead | EventType::kEventWrite | EventType::kEventError);
            this->m_scheduler.lock()->DelObjector(this->m_fd);
        }
    }
    spdlog::info("[{}:{}] [CoBusinessExec BusinessCo Finished]",__FILE__,__LINE__);
    co_return common::CoroutineStatus::kCoroutineFinished;
}

galay::common::GY_NetCoroutine<galay::common::CoroutineStatus> 
galay::kernel::GY_Connector::CoReceiveExec()
{
    auto exit = this->m_exit;
    while(1)
    {
        co_await std::suspend_always{};
        if(*exit) break;
        RealRecv();
    }
    spdlog::info("[{}:{}] [CoReceiveExec RecvCo Exit]",__FILE__,__LINE__);
    co_return common::CoroutineStatus::kCoroutineFinished;
}

void 
galay::kernel::GY_Connector::RealRecv()
{
    while(1){
        if(!m_tempRequest) m_tempRequest = common::GY_RequestFactory<>::GetInstance()->Create(this->m_scheduler.lock()->GetTcpServerBuilder().lock()->GetTypeName(common::kClassNameRequest));
        if(!m_tempRequest) {
            spdlog::error("[{}:{}] [CoReceiveExec Create RequestObj Fail, TypeName: {}]",__FILE__,__LINE__,this->m_scheduler.lock()->GetTcpServerBuilder().lock()->GetTypeName(common::kClassNameRequest));
            break;
        }
        std::string& buffer = m_receiver->GetRBuffer();
        if(buffer.length() == 0) {
            spdlog::info("[{}:{}] [CoReceiveExec Recv Buffer Length = 0]",__FILE__,__LINE__);
            break;
        }
        common::ProtoJudgeType type = m_tempRequest->DecodePdu(buffer);
        if(type == common::ProtoJudgeType::kProtoFinished){
            PushRequest(std::move(m_tempRequest));
            m_tempRequest = nullptr;
        }else if(type == common::ProtoJudgeType::kProtoIncomplete)
        {
            break;
        }else{
            std::string resp = this->m_scheduler.lock()->IllegalFunction();
            this->m_controller->Close();
            PushResponse(std::move(resp));
            break;
        }
    }
}

galay::common::GY_NetCoroutine<galay::common::CoroutineStatus> 
galay::kernel::GY_Connector::CoSendExec()
{
    auto exit = this->m_exit;
    while(1)
    {
        co_await std::suspend_always{};
        if(*exit) break;
        RealSend();
    }
    co_return common::CoroutineStatus::kCoroutineFinished;
}


void 
galay::kernel::GY_Connector::RealSend()
{
    while (!m_responses.empty())
    {
        if(m_sender->WBufferEmpty()){
            m_sender->AppendWBuffer(std::move(m_responses.front()));
            m_responses.pop();
        }
        m_sender->ExecuteTask();
        if(m_sender->WBufferEmpty()){
            if(this->m_controller->IsClosed()){
                this->m_scheduler.lock()->DelEvent(this->m_fd, EventType::kEventRead | EventType::kEventWrite | EventType::kEventError);
                this->m_scheduler.lock()->DelObjector(this->m_fd);
            }else {
                this->m_scheduler.lock()->DelEvent(this->m_fd, EventType::kEventWrite);
                this->m_scheduler.lock()->AddEvent(this->m_fd, EventType::kEventRead | EventType::kEventError);
            }      
        }else{
            this->m_scheduler.lock()->ModEvent(this->m_fd,EventType::kEventRead, EventType::kEventRead | EventType::kEventWrite | EventType::kEventError);
            break;
        }
    }
}

galay::kernel::GY_Connector::~GY_Connector()
{
    spdlog::info("[{}:{}] [~GY_Connector]",__FILE__,__LINE__);
    if(this->m_ssl){
        IOFunction::NetIOFunction::TcpFunction::SSLDestory(this->m_ssl);
        this->m_ssl = nullptr;
    }
    *(this->m_exit) = true;
    this->m_scheduler.lock()->GetCoroutinePool()->EraseCoroutine(this->m_MainCoId);
    this->m_scheduler.lock()->GetCoroutinePool()->EraseCoroutine(this->m_RecvCoId);
    this->m_scheduler.lock()->GetCoroutinePool()->EraseCoroutine(this->m_SendCoId);
    this->m_scheduler.lock()->GetCoroutinePool()->EraseCoroutine(this->m_UserCoId);
}
