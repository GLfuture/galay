#include "Event.h"
#include "Async.h"
#include "EventEngine.h"
#include "Scheduler.h"
#include "Operation.h"
#include "../util/Time.h"
#include <string.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <spdlog/spdlog.h>

namespace galay::event
{
    
CallbackEvent::CallbackEvent(GHandle handle, EventType type, std::function<void(Event*, EventEngine*)> callback)
    : m_handle(handle), m_type(type), m_callback(callback)
{
    
}

void CallbackEvent::HandleEvent(EventEngine *engine)
{
    this->m_callback(this, engine);
}

void CallbackEvent::Free(EventEngine *engine)
{
    engine->DelEvent(this); 
    delete this;
}

CoroutineEvent::CoroutineEvent(GHandle handle, EventEngine* engine, EventType type)
    : m_handle(handle), m_engine(engine), m_type(type)
{
}

void CoroutineEvent::HandleEvent(EventEngine *engine)
{
    eventfd_t val;
    eventfd_read(m_handle, &val);
    std::unique_lock lock(this->m_mtx);
    while (!m_coroutines.empty())
    {
        auto co = m_coroutines.front();
        m_coroutines.pop();
        lock.unlock();
        co->Resume();
        lock.lock();
    }
}

void CoroutineEvent::Free(EventEngine *engine)
{
    engine->DelEvent(this);  
    delete this;
}

void CoroutineEvent::ResumeCoroutine(coroutine::Coroutine *coroutine)
{
    spdlog::debug("CoroutineEvent::ResumeCoroutine [Engine: {}] [Handle:{}]]", m_engine->GetHandle() , m_handle);
    std::unique_lock lock(this->m_mtx);
    m_coroutines.push(coroutine);
    lock.unlock();
    eventfd_write(m_handle, 1);
}

ListenEvent::ListenEvent(GHandle handle, CallbackStore* callback_store, scheduler::EventScheduler* net_scheduler, scheduler::CoroutineScheduler* co_schedule)
    : m_handle(handle), m_callback_store(callback_store), m_net_scheduler(net_scheduler), m_co_scheduler(co_schedule)
{
    async::HandleOption option(this->m_handle);
    option.HandleNonBlock();
}

void ListenEvent::HandleEvent(EventEngine *engine)
{
    CreateTcpSocket(engine);
}

void ListenEvent::Free(EventEngine *engine)
{
    engine->DelEvent(this);  
    delete this;
}

coroutine::Coroutine ListenEvent::CreateTcpSocket(EventEngine *engine)
{
    #if defined(USE_EPOLL)
    while(1)
    {
        async::AsyncTcpSocket* socket = new async::AsyncTcpSocket();
        bool res = co_await socket->InitialHandle(m_handle);
        if( !res ){
            std::string error = socket->GetLastError();
            if( !error.empty() ) {
                spdlog::error("[{}:{}] [[Accept error] [Errmsg:{}]]", __FILE__, __LINE__, error);
            }
            delete socket;
            co_return;
        }
        spdlog::debug("[{}:{}] [[Accept success] [Handle:{}]]", __FILE__, __LINE__, socket->GetHandle());
        if (!socket->GetOption().HandleNonBlock())
        {
            socket->Close();
            spdlog::error("[{}:{}] [[HandleNonBlock error] [Errmsg:{}]]", __FILE__, __LINE__, strerror(errno));
            delete socket;
            co_return;
        }
        this->m_callback_store->Execute(socket, m_net_scheduler, m_co_scheduler);
    }
#endif
    co_return;
}

TimeEvent::Timer::Timer(int64_t during_time, std::function<void(Timer::ptr)> &&func)
{
    this->m_rightHandle = std::forward<std::function<void(Timer::ptr)>>(func);
    SetDuringTime(during_time);
}

int64_t 
TimeEvent::Timer::GetDuringTime()
{
    return this->m_duringTime;
}

int64_t 
TimeEvent::Timer::GetExpiredTime()
{
    return this->m_expiredTime;
}

int64_t 
TimeEvent::Timer::GetRemainTime()
{
    int64_t time = this->m_expiredTime - time::GetCurrentTime();
    return time < 0 ? 0 : time;
}

void 
TimeEvent::Timer::SetDuringTime(int64_t duringTime)
{
    this->m_duringTime = duringTime;
    this->m_expiredTime = time::GetCurrentTime() + duringTime;
    this->m_success = false;
}

void 
TimeEvent::Timer::Execute()
{
    if ( m_cancle.load() ) return;
    this->m_rightHandle(shared_from_this());
    this->m_success = true;
}

void 
TimeEvent::Timer::Cancle()
{
    this->m_cancle = true;
}

// 是否已经完成
bool 
TimeEvent::Timer::Success()
{
    return this->m_success.load();
}

bool 
TimeEvent::Timer::TimerCompare::operator()(const Timer::ptr &a, const Timer::ptr &b)
{
    if (a->GetExpiredTime() > b->GetExpiredTime())
    {
        return true;
    }
    return false;
}

TimeEvent::TimeEvent(GHandle handle)
    : m_handle(handle)
{
    
}

void TimeEvent::HandleEvent(EventEngine *engine)
{
    std::vector<Timer::ptr> timers;
    std::unique_lock lock(this->m_mutex);
    while (! m_timers.empty() && m_timers.top()->GetExpiredTime()  <= time::GetCurrentTime() ) {
        auto timer = m_timers.top();
        m_timers.pop();
        timers.emplace_back(timer);
    }
    lock.unlock();
    for (auto timer : timers)
    {
        timer->Execute();
    }
    UpdateTimers();
}

void TimeEvent::Free(EventEngine *engine)
{
    engine->DelEvent(this);
    delete this;
}

TimeEvent::Timer::ptr TimeEvent::AddTimer(int64_t during_time, std::function<void(Timer::ptr)> &&func)
{
    auto timer = std::make_shared<Timer>(during_time, std::forward<std::function<void(Timer::ptr)>>(func));
    std::unique_lock<std::shared_mutex> lock(this->m_mutex);
    this->m_timers.push(timer);
    lock.unlock();
    UpdateTimers();
    return timer;
}

void TimeEvent::ReAddTimer(int64_t during_time, Timer::ptr timer)
{
    timer->SetDuringTime(during_time);
    timer->m_cancle.store(false);
    std::unique_lock lock(this->m_mutex);
    this->m_timers.push(timer);
    lock.unlock();
    UpdateTimers();
}

void TimeEvent::UpdateTimers()
{
    struct timespec abstime;
    if (m_timers.empty())
    {
        abstime.tv_sec = 0;
        abstime.tv_nsec = 0;
    }
    else
    {
        std::shared_lock lock(this->m_mutex);
        auto timer = m_timers.top();
        lock.unlock();
        int64_t time = timer->GetRemainTime();
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
    timerfd_settime(this->m_handle, 0, &its, nullptr);
}

WaitEvent::WaitEvent(event::EventEngine* engine, async::AsyncTcpSocket* socket, EventType type)
    :m_engine(engine), m_waitco(nullptr), m_type(type), m_socket(socket)
{
}

void WaitEvent::HandleEvent(EventEngine *engine)
{
    scheduler::GetCoroutineScheduler(m_socket->GetHandle() % scheduler::GetCoroutineSchedulerNum())->ResumeCoroutine(m_waitco);
}

void WaitEvent::Free(EventEngine *engine)
{
    engine->DelEvent(this);  
    delete this;
}

GHandle WaitEvent::GetHandle()
{
    return m_socket->GetHandle();;
}

void WaitEvent::SetWaitCoroutine(coroutine::Coroutine *co)
{
    this->m_waitco = co;
}

RecvEvent::RecvEvent(event::EventEngine* engine, async::AsyncTcpSocket *socket)
    :WaitEvent(engine, socket, kEventTypeRead)
{
}

void RecvEvent::HandleEvent(EventEngine *engine)
{
    GHandle handle = this->m_socket->GetHandle();
    char* result = nullptr;
    char buffer[DEFAULT_READ_BUFFER_SIZE] = {0};
    int recvBytes = 0;
    bool initial = true;
    do{
        bzero(buffer, sizeof(buffer));
        int ret = recv(handle, buffer, DEFAULT_READ_BUFFER_SIZE, 0);
        if( ret == 0 ) {
            m_waitco->SetContext(0);
            break;
        }
        if( ret == -1 ) {
            if( errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR ){
                m_waitco->SetContext(recvBytes);
                break;
            }else{
                m_waitco->SetContext(-1);
                std::string error = "recv error, ";
                error += strerror(errno);
                m_socket->SetLastError(error);
                break;
            }
        } else {
            if(initial) {
                result = new char[ret];
                bzero(result, ret);
                initial = false;
            }else{
                result = (char*)realloc(result, ret + recvBytes);
            }
            memcpy(result + recvBytes, buffer, ret);
            recvBytes += ret;
        }
    } while(1); 
    if( recvBytes > 0){
        spdlog::debug("RecvEvent::HandleEvent [Engine: {}] [Handle: {}] [Byte: {}] [Data: {}]", engine->GetHandle(), handle, recvBytes, std::string_view(result, recvBytes));   
        std::string_view origin = m_socket->GetRBuffer();
        char* new_buffer = nullptr;
        if(!origin.empty()) {
            new_buffer = new char[origin.size() + recvBytes];
            memcpy(new_buffer, origin.data(), origin.size());
            memcpy(new_buffer + origin.size(), result, recvBytes);
            delete [] origin.data();
            delete [] result;
        }else{
            new_buffer = result;
        }
        m_socket->SetRBuffer(std::string_view(new_buffer, recvBytes + origin.size()));
    }
    //2.唤醒协程
    scheduler::GetCoroutineScheduler(handle % scheduler::GetCoroutineSchedulerNum())->ResumeCoroutine(m_waitco);
}

SendEvent::SendEvent(event::EventEngine* engine, async::AsyncTcpSocket *socket)
    :WaitEvent(engine, socket, kEventTypeWrite)
{
}

void SendEvent::HandleEvent(EventEngine *engine)
{
    GHandle handle = this->m_socket->GetHandle();
    int sendBytes = 0;
    std::string_view wbuffer = m_socket->MoveWBuffer();
    do {
        std::string_view view = wbuffer.substr(sendBytes);
        int ret = send(handle, view.data(), view.size(), 0);
        if( ret == -1 ) {
            if( errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR ){
                m_waitco->SetContext(sendBytes);
                break;
            }else{
                m_waitco->SetContext(-1);
                std::string error = "send error, ";
                error += strerror(errno);
                m_socket->SetLastError(error);
                break;
            }
        }
        sendBytes += ret;
        if( sendBytes == wbuffer.size() ) {
            m_waitco->SetContext(sendBytes);
            break;
        }
    } while (1);
    m_socket->SetWBuffer(wbuffer.substr(sendBytes));
    spdlog::debug("RecvEvent::HandleEvent [Engine: {}] [Handle: {}] [Byte: {}] [Data: {}]", engine->GetHandle(), handle, sendBytes, wbuffer.substr(0, sendBytes));
    scheduler::GetCoroutineScheduler(handle % scheduler::GetCoroutineSchedulerNum())->ResumeCoroutine(m_waitco);
}

}