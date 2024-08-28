#include "poller.h"
#include "server.h"
#include "builder.h"
#include "../common/coroutine.h"
#include "../util/io.h"
#include <sys/select.h>
#include <spdlog/spdlog.h>

namespace galay::poller
{

static std::function<void(server::Controller::ptr)> g_wrongHandle;
static std::function<void(server::Controller::ptr)> g_rightHandle;

void 
SetWrongHandle(std::function<void(server::Controller::ptr)> handle)
{
    g_wrongHandle = handle;
}

void 
SetRightHandle(std::function<void(server::Controller::ptr)> handle)
{
    g_rightHandle = handle;
}

int 
RecvAll(IOScheduler::ptr scheduler, int fd, SSL* ssl, std::string& recvBuffer)
{
    char buffer[DEFAULT_RBUFFER_LENGTH] = {0};
    while (1)
    {
        bzero(buffer, DEFAULT_RBUFFER_LENGTH);
        ssize_t len;
        if(!ssl){
            len = io::net::TcpFunction::Recv(fd, buffer, DEFAULT_RBUFFER_LENGTH);
        }else{
            len = io::net::TcpFunction::SSLRecv(ssl, buffer, DEFAULT_RBUFFER_LENGTH);
        }
        spdlog::debug("[{}:{}] [Recv Len:{}, Data:{}]", __FILE__, __LINE__, len, buffer);
        if (len == -1)
        {
            if (errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN)
            {
                spdlog::error("[{}:{}] [Fail(fd:{})] [Errmsg:{}]", __FILE__, __LINE__, fd, strerror(errno));
                close(fd);
                scheduler->DelObjector(fd);
                scheduler->DelEvent(fd, poller::kEventError | poller::kEventRead | poller::kEventWrite);
                return -1;
            }
            else
            {
                break;
            }
        }
        else if (len == 0)
        {
            spdlog::info("[{}:{}] [Recv warn(fd:{})] [The peer closes the connection]", __FILE__, __LINE__, fd);
            scheduler->DelEvent(fd, poller::kEventError | poller::kEventRead | poller::kEventWrite);
            scheduler->DelObjector(fd);
            return -1;
        }
        else
        {
            recvBuffer.append(buffer, len);
        }
    }
    return 0;
}

int 
SendAll(IOScheduler::ptr scheduler, int fd, SSL* ssl, std::string& buffer)
{
    int offset = 0;
    while (1)
    {
        if (buffer.length() == offset)
        {
            buffer.clear();
            break;
        }
        size_t len;
        if(!ssl){
            len = io::net::TcpFunction::Send(fd, buffer.data() + offset, buffer.length() - offset);
        }else{
            len = io::net::TcpFunction::SSLSend(ssl, buffer.data() + offset, buffer.length() - offset);
        }
        spdlog::debug("[{}:{}] [Send Len:{}, Data:{}]", __FILE__, __LINE__, len, buffer.substr(offset, len));
        if (len == -1)
        {
            if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
            {
                spdlog::error("[{}:{}] [Send error(fd:{})] [Errmsg:{}]", __FILE__, __LINE__, fd, strerror(errno));
                close(fd);
                scheduler->DelObjector(fd);
                scheduler->DelEvent(fd, poller::kEventError | poller::kEventRead | poller::kEventWrite);
                return -1;
            }
            else
            {
                buffer.erase(0, offset);
                break;
            }
        }
        else if (len == 0)
        {
            spdlog::info("[{}:{}] [Send info(fd:{})] [The peer closes the connection]", __FILE__, __LINE__, fd);
            scheduler->DelObjector(fd);
            scheduler->DelEvent(fd, poller::kEventError | poller::kEventRead | poller::kEventWrite);
            return -1;
        }
        else
        {
            offset += len;
        }
    }
    return 0;
}

static int 
CreateTcpFd(uint16_t port, uint16_t backlog)
{
    int fd = io::net::TcpFunction::Sock();
    if (fd <= 0)
    {
        spdlog::error("[{}:{}] [[Sock error] [Errmsg:{}]]", __FILE__, __LINE__, strerror(errno));
        return -1;
    }
    int ret = io::net::TcpFunction::ReuseAddr(fd);
    if (ret == -1)
    {
        spdlog::error("[{}:{}] [[ReuseAddr(fd: {}) error] [Errmsg:{}]]", __FILE__, __LINE__, fd, strerror(errno));
        close(fd);
        return -1;
    }
    ret = io::net::TcpFunction::ReusePort(fd);
    ret = io::net::TcpFunction::Bind(fd, port);
    if (ret == -1)
    {
        spdlog::error("[{}:{}] [[Bind(fd: {}) error] [Errmsg:{}]]", __FILE__, __LINE__, fd, strerror(errno));
        close(fd);
        return -1;
    }
    ret = io::net::TcpFunction::Listen(fd, backlog);
    if (ret == -1)
    {
        spdlog::error("[{}:{}] [[Listen(fd: {}) error] [Errmsg:{}]]", __FILE__, __LINE__, fd, strerror(errno));
        close(fd);
        return -1;
    }
    ret = io::net::TcpFunction::SetNoBlock(fd);
    if (ret == -1)
    {
        spdlog::error("[{}:{}] [[SetNoBlock(fd: {}) error] [Errmsg:{}]]", __FILE__, __LINE__, fd, strerror(errno));
        close(fd);
        return -1;
    }
    return fd;
}

static int 
CreateConn(IOScheduler::ptr scheduler, int fd, SSL_CTX* sslCtx, std::string requestName)
{
    while(1)
    {
        int connfd = io::net::TcpFunction::Accept(fd);
        if (connfd == -1)
        {
            if (errno != EINTR && errno != EWOULDBLOCK && errno != ECONNABORTED && errno != EPROTO)
            {
                spdlog::error("[{}:{}] [[Accept error(fd:{})] [Errmsg:{}]]", __FILE__, __LINE__, fd, strerror(errno));
                return -1;
            }
            else
            {
                break;
            }
        }
        spdlog::debug("[{}:{}] [[Accept success(fd:{})] [Fd:{}]]", __FILE__, __LINE__, fd, connfd);
        int ret = io::BlockFuction::SetNoBlock(connfd);
        if (ret == -1)
        {
            close(connfd);
            spdlog::error("[{}:{}] [[SetNoBlock error(fd:{})] [Errmsg:{}]]", __FILE__, __LINE__, fd, strerror(errno));
            return -1;
        }
        spdlog::debug("[{}:{}] [SetNoBlock Success(fd:{})]", __FILE__, __LINE__, connfd);
        SSL* ssl = nullptr;
        if(sslCtx)
        {
            ssl = io::net::TcpFunction::SSLCreateObj(sslCtx,connfd);
            int retryTimes = 0;
            do{
                ret = io::net::TcpFunction::SSLAccept(ssl);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                if(retryTimes++ > 4000){
                    int ssl_err = SSL_get_error(ssl, ret);
                    char msg[256];
                    ERR_error_string(ssl_err, msg);
                    spdlog::error("[{}:{}] [socket(fd: {}) SSL_Accept error: '{}']", __FILE__, __LINE__, connfd, msg);
                    io::net::TcpFunction::SSLDestory(ssl);
                    ssl = nullptr;
                    spdlog::error("[{}:{}] [socket(fd: {}) close]", __FILE__, __LINE__, connfd);
                    scheduler->DelObjector(connfd);
                    close(connfd);
                    return -1;
                }
                if (ret <= 0)
                {
                    int ssl_err = SSL_get_error(ssl, ret);
                    if (ssl_err == SSL_ERROR_WANT_READ || ssl_err == SSL_ERROR_WANT_WRITE || ssl_err == SSL_ERROR_WANT_ACCEPT)
                    {
                        continue;
                    }
                    else
                    {
                        char msg[256];
                        ERR_error_string(ssl_err, msg);
                        spdlog::error("[{}:{}] [socket(fd: {}) SSL_Accept error: '{}']", __FILE__, __LINE__, connfd, msg);
                        io::net::TcpFunction::SSLDestory(ssl);
                        ssl = nullptr;
                        spdlog::error("[{}:{}] [socket(fd: {}) close]", __FILE__, __LINE__, connfd);
                        scheduler->DelObjector(connfd);
                        close(connfd);
                        return -1;
                    }
                }
            }while (ret <= 0);
        }
        TcpConnector::ptr connector = std::make_shared<TcpConnector>(connfd, ssl, scheduler, requestName);
        scheduler->RegisterObjector(connfd, connector);
        if (scheduler->AddEvent(connfd, poller::kEventRead | poller::kEventEpollET | poller::kEventError) == -1)
        {
            close(connfd);
            spdlog::error("[{}:{}] [AddEvent error(fd:{})] [Errmsg:{}]", __FILE__, __LINE__, connfd, strerror(errno));
            return -1;
        }
    }
    return 0;
}

Callback &
Callback::operator+=(const std::function<void()> &callback)
{
    m_callback = callback;
    return *this;
}

void 
Callback::operator()()
{
    m_callback();
}

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
void 
Timer::Cancle()
{
    this->m_cancle = true;
}

bool 
Timer::IsCancled()
{
    return this->m_cancle;
}

// 是否已经完成
bool 
Timer::Success()
{
    return this->m_success.load();
}

bool 
TimerCompare::operator()(const Timer::ptr &a, const Timer::ptr &b)
{
    if (a->GetExpiredTime() > b->GetExpiredTime())
    {
        return true;
    }
    else if (a->GetExpiredTime() < b->GetExpiredTime())
    {
        return false;
    }
    return a->GetTimerId() > b->GetTimerId();
}

std::atomic_uint64_t TimerManager::m_global_timerid = 0;

TimerManager::TimerManager()
{
    this->m_timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
    this->m_readCallback += [this](){
        Timer::ptr timer = GetEaliestTimer();
        timer->Execute();
    };
}

Timer::ptr
TimerManager::AddTimer(uint64_t during, uint32_t exec_times, std::function<void(Timer::ptr)> &&func)
{
    m_global_timerid.fetch_add(1, std::memory_order_acquire);
    std::unique_lock<std::shared_mutex> lock(this->m_mtx);
    uint64_t timerid = m_global_timerid.load(std::memory_order_acquire);
    if (timerid >= MAX_TIMERID)
    {
        m_global_timerid.store((timerid % MAX_TIMERID), std::memory_order_release);
    }
    auto timer = std::make_shared<Timer>(timerid, during, exec_times, std::forward<std::function<void(Timer::ptr)>>(func));
    this->m_timers.push(timer);
    lock.unlock();
    UpdateTimerfd();
    return timer;
}

void 
TimerManager::UpdateTimerfd()
{
    struct timespec abstime;
    if (m_timers.empty())
    {
        abstime.tv_sec = 0;
        abstime.tv_nsec = 0;
    }
    else
    {
        std::shared_lock lock(this->m_mtx);
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
    timerfd_settime(this->m_timerfd, 0, &its, nullptr);
}

Timer::ptr 
TimerManager::GetEaliestTimer()
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
TimerManager::GetTimerfd(){ 
    return this->m_timerfd;  
}

Callback& 
TimerManager::OnRead()
{
    return m_readCallback;
}

Callback& 
TimerManager::OnWrite()
{
    return m_sendCallback;
}


TimerManager::~TimerManager()
{
    while (!m_timers.empty())
    {
        m_timers.pop();
    }
}

TcpAcceptor::TcpAcceptor(std::weak_ptr<IOScheduler> manager, uint16_t port, uint16_t backlog, std::string requestName, io::net::SSLConfig::ptr config)
{
    this->m_listenFd = CreateTcpFd(port, backlog);
    this->m_sslCtx = nullptr;
    if(config){
        long minVersion = config->GetMinSSLVersion();
        long maxVersion = config->GetMaxSSLVersion();
        this->m_sslCtx = io::net::TcpFunction::InitSSLServer(minVersion,maxVersion);
        if(!m_sslCtx) throw std::runtime_error("InitSSLServer failed");
        if(io::net::TcpFunction::SetSSLCertAndKey(m_sslCtx, config->GetCertPath().c_str(), config->GetKeyPath().c_str()) == -1){
            throw std::runtime_error("SetSSLCertAndKey failed");
        };
    }
    this->m_readCallback += [manager, requestName, this](){
        int ret = CreateConn(manager.lock(), m_listenFd, this->m_sslCtx, requestName);
        if(ret == -1) {
            this->m_errMsg = strerror(errno);
        } else {
            if( !this->m_errMsg.empty()) this->m_errMsg.clear();
        }
    };
}

int 
TcpAcceptor::GetListenFd()
{
    return this->m_listenFd;
}

Callback& 
TcpAcceptor::OnRead()
{
    return m_readCallback;
}

Callback& 
TcpAcceptor::OnWrite()
{
    return m_sendCallback;
}

TcpAcceptor::~TcpAcceptor()
{
    if(this->m_sslCtx) 
    {
        io::net::TcpFunction::SSLDestory({}, this->m_sslCtx);
        this->m_sslCtx = nullptr;
    }
}

TcpConnector::TcpConnector(int fd, SSL* ssl, std::weak_ptr<IOScheduler> scheduler, std::string requestName)
{
    this->m_fd = fd;
    this->m_ssl = ssl;
    this->m_isClosed = false;
    this->m_scheduler = scheduler;
    this->m_controller = nullptr;
    this->m_requestName = requestName;
    this->m_readCallback += [this](){
        RealRecv();
    };
}

void 
TcpConnector::Close()
{
    if(!m_isClosed) 
    {
        spdlog::info("[{}:{}] [Close Connection(fd: {})]", __FILE__, __LINE__, this->m_fd);
        this->m_scheduler.lock()->DelEvent(this->m_fd, poller::kEventRead | poller::kEventWrite | poller::kEventError);
        this->m_scheduler.lock()->DelObjector(this->m_fd);
        io::BlockFuction::SetBlock(this->m_fd);
        close(this->m_fd);
        this->m_isClosed = true;
    }
}

std::shared_ptr<Timer> 
TcpConnector::AddTimer(uint64_t during, uint32_t exec_times,std::function<void(std::shared_ptr<Timer>)> &&func)
{
    return this->m_scheduler.lock()->GetTimerManager()->AddTimer(during,exec_times,std::forward<std::function<void(std::shared_ptr<Timer>)>>(func));
}

galay::protocol::Request::ptr 
TcpConnector::GetRequest()
{
    if(m_requests.empty()) return nullptr;
    return m_requests.front();
}

void 
TcpConnector::PopRequest()
{
    if(!m_requests.empty()) m_requests.pop();
}

bool 
TcpConnector::HasRequest()
{
    return !m_requests.empty();
}

Callback& 
TcpConnector::OnRead()
{
    return m_readCallback;
}

Callback&
TcpConnector::OnWrite()
{
    return m_sendCallback;
}

result::ResultInterface::ptr 
TcpConnector::Send(std::string&& response)
{
    this->m_wbuffer = std::forward<std::string>(response);
    result::ResultInterface::ptr result = std::make_shared<result::ResultInterface>();
    int ret = SendAll(this->m_scheduler.lock(), this->m_fd, this->m_ssl, this->m_wbuffer);
    if(this->m_wbuffer.empty()){
        result->SetSuccess(true);
        goto end;
    }else{
        result->AddTaskNum(1);
        result->SetErrorMsg("Waiting");
        m_sendCallback += [result,this](){
            RealSend(result);
        };
        this->m_scheduler.lock()->ModEvent(this->m_fd, poller::kEventRead, poller::kEventWrite);
    }
end:
    return result;
}


void 
TcpConnector::RealSend(std::shared_ptr<result::ResultInterface> result)
{
    int ret = SendAll(this->m_scheduler.lock(), this->m_fd, this->m_ssl, this->m_wbuffer);
    if(this->m_wbuffer.empty())
    {
        this->m_scheduler.lock()->ModEvent(this->m_fd, poller::kEventWrite, poller::kEventRead);
        if(result) {
            result->SetSuccess(true);
            result->Done();
        }
    }
}

void 
TcpConnector::RealRecv()
{
    if(!this->m_controller) {
        this->m_controller = std::make_shared<server::Controller>(shared_from_this());
    }
    int ret = RecvAll(this->m_scheduler.lock(), this->m_fd, this->m_ssl, this->m_rbuffer);
    while(true)
    {
        if(!m_tempRequest) m_tempRequest = common::RequestFactory<>::GetInstance()->Create(this->m_requestName);
        if(!m_tempRequest) 
        {
            spdlog::error("[{}:{}] [CoReceiveExec Create RequestObj Fail, TypeName: {}]",__FILE__,__LINE__, this->m_requestName);
            return;
        }
        if(m_rbuffer.empty()) break;
        spdlog::debug("[{}:{}] [CoReceiveExec Recv Buffer: {}]",__FILE__,__LINE__, m_rbuffer);
        int eLen = m_tempRequest->DecodePdu(m_rbuffer);
        if(m_tempRequest->ParseSuccess())
        {
            m_rbuffer.erase(0, eLen);
            m_requests.push(m_tempRequest);
            m_tempRequest = common::RequestFactory<>::GetInstance()->Create(this->m_requestName);
        }
        else if(m_tempRequest->ParseIncomplete())
        {
            m_rbuffer.erase(0, eLen);
            break;
        }
        else if(m_tempRequest->ParseIllegal())
        {
            m_requests.push(m_tempRequest);
            m_tempRequest = common::RequestFactory<>::GetInstance()->Create(m_requestName);
            g_wrongHandle(this->m_controller);
            break;
        }
    }
    this->m_scheduler.lock()->ModEvent(this->m_fd, poller::kEventRead, poller::kEventRead);
    if(m_requests.empty()) return;
    g_rightHandle(this->m_controller);
}

TcpConnector::~TcpConnector()
{
    if(this->m_ssl){
        io::net::TcpFunction::SSLDestory(this->m_ssl);
        this->m_ssl = nullptr;
    }
}

Callback& 
ClientExcutor::OnRead() 
{
    return this->m_readCallback;
}

Callback& 
ClientExcutor::OnWrite()
{
    return this->m_sendCallback;
}


SelectScheduler::SelectScheduler(uint64_t timeout)
{
    FD_ZERO(&m_rfds);
    FD_ZERO(&m_wfds);
    FD_ZERO(&m_efds);
    this->m_stop = false;
    this->m_timerfd = 0;
    this->m_timeout = timeout;
    this->m_maxfd = 0;
}

void 
SelectScheduler::RegisterObjector(int fd, Objector::ptr objector)
{
    m_objectors[fd] = objector;
}

void 
SelectScheduler::RegiserTimerManager(int fd, TimerManager::ptr timerManager)
{
    this->m_timerfd = fd;
    RegisterObjector(fd,timerManager);
    AddEvent(m_timerfd, EventType::kEventError | EventType::kEventRead);
}

void 
SelectScheduler::DelObjector(int fd)
{
    this->m_eraseQueue.push(fd);
}

bool 
SelectScheduler::RealDelObjector(int fd)
{
    if(m_objectors.contains(fd)){
        this->m_objectors.erase(fd);
        spdlog::debug("[{}:{}] [RealDelObjector(fd :{})]", __FILE__, __LINE__, fd);
        return true;
    }
    return false;
}

int 
SelectScheduler::DelEvent(int fd, int event_type)
{
    if(fd >= __FD_SETSIZE) return -1;
    if ((event_type & kEventRead) != 0)
    {
        FD_CLR(fd, &m_rfds);
    }
    if ((event_type & kEventWrite) != 0)
    {
        FD_CLR(fd, &m_wfds);
    }
    if ((event_type & kEventError) != 0)
    {
        FD_CLR(fd, &m_efds);
    }
    if (this->m_maxfd == fd)
        this->m_maxfd--;
    else if (this->m_minfd == fd)
        this->m_minfd++;
    return 0;
}

int
SelectScheduler::ModEvent(int fd, int from ,int to)
{
    if(fd >= __FD_SETSIZE) return -1;
    int ret = DelEvent(fd, from);
    if(ret == -1) return ret;
    ret = AddEvent(fd, to);
    return ret;
}


int 
SelectScheduler::AddEvent(int fd, int event_type)
{
    if(fd >= __FD_SETSIZE) return -1;
    if ((event_type & kEventRead) != 0)
    {
        FD_SET(fd, &m_rfds);
    }
    if ((event_type & kEventWrite) != 0)
    {
        FD_SET(fd, &m_wfds);
    }
    if ((event_type & kEventError) != 0)
    {
        FD_SET(fd, &m_efds);
    }
    this->m_maxfd = std::max(this->m_maxfd, fd);
    this->m_minfd = std::min(this->m_minfd, fd);
    return 0;
}


void
SelectScheduler::Stop()
{
    if (!m_stop)
    {
        FD_ZERO(&m_rfds);
        FD_ZERO(&m_wfds);
        FD_ZERO(&m_efds);
        for (auto it = m_objectors.begin(); it != m_objectors.end(); ++it)
        {
            this->DelEvent(it->first, kEventRead | kEventWrite | kEventError);
            close(it->first);
        }
        this->m_objectors.clear();
        this->m_stop = true;
    }
}

TimerManager::ptr 
SelectScheduler::GetTimerManager()
{
    return std::dynamic_pointer_cast<TimerManager>(m_objectors[m_timerfd]);
}

void 
SelectScheduler::Start()
{
    fd_set read_set, write_set, excep_set;
    timeval tv;
    tv.tv_sec = this->m_timeout / 1000;
    tv.tv_usec = this->m_timeout % 1000 * 1000;
    while (!m_stop)
    {
        memcpy(&read_set, &m_rfds, sizeof(fd_set));
        memcpy(&write_set, &m_wfds, sizeof(fd_set));
        memcpy(&excep_set, &m_efds, sizeof(fd_set));
        int nready = select(this->m_maxfd + 1, &read_set, &write_set, &excep_set, &tv);
        if (m_stop)
            break;
        if (nready == -1)
        {
            spdlog::error("[{}:{}] [[select(tid: {})] [Errmsg:{}]]",__FILE__,__LINE__,std::hash<std::thread::id>{}(std::this_thread::get_id()),strerror(errno));
            return;
        }
        while(!m_eraseQueue.empty())
        {
            int fd = m_eraseQueue.front();
            m_eraseQueue.pop();
            RealDelObjector(fd);
        }
        if (nready != 0){
            for (int fd = m_minfd; fd > 0 && fd <= m_maxfd; fd++)
            {
               
                auto objector = m_objectors[fd];
                if(objector){
                    if(FD_ISSET(fd, &read_set))
                    {
                        spdlog::debug("[{}:{}] [fd:{}, OnRead]", __FILE__, __LINE__, fd);
                        objector->OnRead()();
                    }
                    else if(FD_ISSET(fd, &write_set))
                    {
                        spdlog::debug("[{}:{}] [fd:{}, OnWrite]", __FILE__, __LINE__, fd);
                        objector->OnWrite()();
                    }
                }
            }
        }
        tv.tv_sec = this->m_timeout / 1000;
        tv.tv_usec = this->m_timeout % 1000 * 1000;
    } 
}

SelectScheduler::~SelectScheduler()
{
    if(!m_stop){
        Stop();
    }
}

EpollScheduler::EpollScheduler(uint32_t eventSize, uint64_t timeout)
{
    this->m_epollfd = epoll_create(1);
    this->m_events = new epoll_event[eventSize];
    this->m_stop = false;
    this->m_timeout = timeout;
    this->m_eventSize = eventSize;
}

void 
EpollScheduler::RegisterObjector(int fd, Objector::ptr objector)
{
    this->m_objectors[fd] = objector;
}

void 
EpollScheduler::RegiserTimerManager(int fd, TimerManager::ptr timerManager)
{
    this->m_timerfd = fd;
    RegisterObjector(fd,timerManager);
    AddEvent(m_timerfd, EventType::kEventError | EventType::kEventRead);
}

void 
EpollScheduler::DelObjector(int fd) 
{
    m_eraseQueue.push(fd);
}


bool 
EpollScheduler::RealDelObjector(int fd)
{
    if(m_objectors.contains(fd)){
        m_objectors.erase(fd);
        spdlog::debug("[{}:{}] [DelObjector(fd :{})]", __FILE__, __LINE__, fd);
        return true;
    }
    return false;
}

void 
EpollScheduler::Start() 
{
    while (!m_stop)
    {
        int nready = epoll_wait(this->m_epollfd, m_events, m_eventSize, m_timeout);
        if(m_stop) break;
        if(nready < 0){
            spdlog::error("[{}:{}] [[epoll_wait error(tid: {})] [Errmsg:{}]]",__FILE__,__LINE__,std::hash<std::thread::id>{}(std::this_thread::get_id()),strerror(errno));
            return;
        }
        while (!this->m_eraseQueue.empty())
        {
            int fd = this->m_eraseQueue.front();
            this->m_eraseQueue.pop();
            RealDelObjector(fd);
        }
        while(--nready >= 0){
            int fd = m_events[nready].data.fd;
            auto objector = m_objectors[fd];
            if(objector){
                if(m_events[nready].events & EPOLLIN)
                {
                    objector->OnRead()();
                }
                else if(m_events[nready].events & EPOLLOUT)
                {
                    objector->OnWrite()();
                }
            }
        }
    }
}

int 
EpollScheduler::DelEvent(int fd, int event_type) 
{
    epoll_event ev = {0};
    ev.data.fd = fd;
    if((event_type & kEventRead) != 0)
    {
        ev.events |= EPOLLIN;
    }
    if((event_type & kEventWrite) != 0)
    {
        ev.events |= EPOLLOUT;
    }
    if((event_type & kEventError) != 0)
    {
        ev.events |= EPOLLRDHUP;
    }
    return epoll_ctl(m_epollfd, EPOLL_CTL_DEL, fd, &ev);
}

int 
EpollScheduler::ModEvent(int fd, int from ,int to) 
{
    epoll_event ev = {0};
    ev.data.fd = fd;
    if((to & kEventRead) != 0)
    {
        ev.events |= EPOLLIN;
    }
    if((to & kEventWrite) != 0)
    {
        ev.events |= EPOLLOUT;
    }
    if((to & kEventError) != 0)
    {
        ev.events |= EPOLLRDHUP;
    }
    return epoll_ctl(m_epollfd, EPOLL_CTL_MOD, fd, &ev);
}

int 
EpollScheduler::AddEvent(int fd, int event_type) 
{
    epoll_event ev = {0};
    ev.data.fd = fd;
    if((event_type & kEventRead) != 0)
    {
        ev.events |= EPOLLIN;
    }
    if((event_type & kEventWrite) != 0)
    {
        ev.events |= EPOLLOUT;
    }
    if((event_type & kEventError) != 0)
    {
        ev.events |= EPOLLRDHUP;
    }
    if((event_type & kEventEpollET) != 0)
    {
        ev.events |= EPOLLET;
    }
    if((event_type & kEventEpollOneShot) != 0)
    {
        ev.events |= EPOLLONESHOT;
    }
    return epoll_ctl(m_epollfd, EPOLL_CTL_ADD, fd, &ev);
}

void 
EpollScheduler::Stop() 
{
    if (!m_stop)
    {
        for (auto it = m_objectors.begin(); it != m_objectors.end(); ++it)
        {
            this->DelEvent(it->first, kEventRead | kEventWrite | kEventError);
            close(it->first);
        }
        this->m_objectors.clear();
        this->m_stop = true;
    }
}

TimerManager::ptr 
EpollScheduler::GetTimerManager()
{
    return std::dynamic_pointer_cast<TimerManager>(m_objectors[m_timerfd]);
}


EpollScheduler::~EpollScheduler()
{
    close(this->m_epollfd);
    delete [] this->m_events;
}
}