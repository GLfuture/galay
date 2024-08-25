#include "task.h"
#include "scheduler.h"
#include "builder.h"
#include "server.h"
#include "objector.h"
#include "../util/io.h"
#include <spdlog/spdlog.h>

namespace galay::task
{
GY_TcpCreateConnTask::GY_TcpCreateConnTask(std::weak_ptr<server::GY_SIOManager> ioManager)
{
    this->m_ioManager = ioManager;
    this->m_success = false;
    this->m_fd = CreateListenFd(this->m_ioManager.lock()->GetTcpServerBuilder());
    if(this->m_fd < 0) throw std::runtime_error("CreateListenFd error");
}

void 
GY_TcpCreateConnTask::Execute()
{
    while(1)
    {
        if(CreateConn() == -1) break;
    }
}

bool 
GY_TcpCreateConnTask::Success() 
{
    return m_success;
}

std::string 
GY_TcpCreateConnTask::Error()
{
    return m_error;
}

int 
GY_TcpCreateConnTask::CreateConn()
{
    int connfd = io::net::TcpFunction::Accept(this->m_fd);
    if (connfd == -1)
    {
        if (errno != EINTR && errno != EWOULDBLOCK && errno != ECONNABORTED && errno != EPROTO)
        {
            spdlog::error("[{}:{}] [[Accept error(fd:{})] [Errmsg:{}]]", __FILE__, __LINE__, this->m_fd, strerror(errno));
            if(m_success) m_success = false;
            m_error = strerror(errno);
        }
        else
        {
            spdlog::debug("[{}:{}] [[Accept warn(fd:{})] [Errmsg:{}]]", __FILE__, __LINE__, this->m_fd, strerror(errno));
            if(m_success) m_success = false;
            m_error = strerror(errno);
        }
        return -1;
    }
    spdlog::debug("[{}:{}] [[Accept success(fd:{})] [Fd:{}]]", __FILE__, __LINE__, this->m_fd, connfd);
    int ret = io::BlockFuction::SetNoBlock(connfd);
    if (ret == -1)
    {
        close(connfd);
        spdlog::error("[{}:{}] [[SetNoBlock error(fd:{})] [Errmsg:{}]]", __FILE__, __LINE__, this->m_fd, strerror(errno));
        if(m_success) m_success = false;
        m_error = strerror(errno);
        return -1;
    }
    spdlog::debug("[{}:{}] [SetNoBlock Success(fd:{})]", __FILE__, __LINE__, connfd);
    std::shared_ptr<objector::GY_TcpConnector> connector = std::make_shared<objector::GY_TcpConnector>(connfd, nullptr, this->m_ioManager);
    m_ioManager.lock()->GetIOScheduler()->RegisterObjector(connfd, connector);
    
    if (m_ioManager.lock()->GetIOScheduler()->AddEvent(connfd, poller::kEventRead | poller::kEventEpollET | poller::kEventError) == -1)
    {
        close(connfd);
        spdlog::error("[{}:{}] [AddEvent error(fd:{})] [Errmsg:{}]", __FILE__, __LINE__, connfd, strerror(errno));
        if(m_success) m_success = false;
        m_error = strerror(errno);
        return -1;
    }
    m_success = true;
    if(!m_error.empty()) m_error.clear();
    return connfd;
}

int 
GY_TcpCreateConnTask::CreateListenFd(std::weak_ptr<server::GY_TcpServerBuilderBase> builder)
{
    int fd = io::net::TcpFunction::Sock();
    if (fd <= 0)
    {
        if(m_success) m_success = false;
        m_error = strerror(errno);
        spdlog::error("[{}:{}] [[Sock error] [Errmsg:{}]]", __FILE__, __LINE__, strerror(errno));
        return -1;
    }
    int ret = io::net::TcpFunction::ReuseAddr(fd);
    if (ret == -1)
    {
        if(m_success) m_success = false;
        m_error = strerror(errno);
        spdlog::error("[{}:{}] [[ReuseAddr(fd: {}) error] [Errmsg:{}]]", __FILE__, __LINE__, fd, strerror(errno));
        close(fd);
        return -1;
    }
    ret = io::net::TcpFunction::ReusePort(fd);
    ret = io::net::TcpFunction::Bind(fd, builder.lock()->GetPort());
    if (ret == -1)
    {
        if(m_success) m_success = false;
        m_error = strerror(errno);
        spdlog::error("[{}:{}] [[Bind(fd: {}) error] [Errmsg:{}]]", __FILE__, __LINE__, fd, strerror(errno));
        close(fd);
        return -1;
    }
    ret = io::net::TcpFunction::Listen(fd, builder.lock()->GetBacklog());
    if (ret == -1)
    {
        if(m_success) m_success = false;
        m_error = strerror(errno);
        spdlog::error("[{}:{}] [[Listen(fd: {}) error] [Errmsg:{}]]", __FILE__, __LINE__, fd, strerror(errno));
        close(fd);
        return -1;
    }
    ret = io::net::TcpFunction::SetNoBlock(fd);
    if (ret == -1)
    {
        if(m_success) m_success = false;
        m_error = strerror(errno);
        spdlog::error("[{}:{}] [[SetNoBlock(fd: {}) error] [Errmsg:{}]]", __FILE__, __LINE__, fd, strerror(errno));
        close(fd);
        return -1;
    }
    return fd;
}


int 
GY_TcpCreateConnTask::GetFd()
{
    return this->m_fd;
}

GY_TcpRecvTask::GY_TcpRecvTask(int fd, SSL* ssl, std::weak_ptr<poller::GY_IOScheduler> scheduler)
{
    this->m_fd = fd;
    this->m_ssl = ssl;
    this->m_scheduler = scheduler;
    this->m_success = false;
}


void 
GY_TcpRecvTask::RecvAll()
{
    Execute();
}

bool 
GY_TcpRecvTask::Success()
{
    return this->m_success;
}

std::string 
GY_TcpRecvTask::Error()
{
    return this->m_error;
}

std::string&
GY_TcpRecvTask::GetRBuffer()
{
    return m_rbuffer;
}


void 
GY_TcpRecvTask::Execute()
{
    char buffer[DEFAULT_RBUFFER_LENGTH] = {0};
    while (1)
    {
        bzero(buffer, DEFAULT_RBUFFER_LENGTH);
        ssize_t len;
        if(!m_ssl){
            len = io::net::TcpFunction::Recv(this->m_fd, buffer, DEFAULT_RBUFFER_LENGTH);
        }else{
            len = io::net::TcpFunction::SSLRecv(this->m_ssl,buffer,DEFAULT_RBUFFER_LENGTH);
        }
        if (len == -1)
        {
            if (errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN)
            {
                if(m_success) m_success = false;
                m_error = strerror(errno);
                spdlog::error("[{}:{}] [Fail(fd:{})] [Errmsg:{}]", __FILE__, __LINE__, this->m_fd, strerror(errno));
                close(this->m_fd);
                this->m_scheduler.lock()->DelObjector(this->m_fd);
                this->m_scheduler.lock()->DelEvent(this->m_fd, poller::kEventError | poller::kEventRead | poller::kEventWrite);
            }
            else
            {
                m_success = true;
                if(!m_error.empty()) m_error.clear();
                spdlog::debug("[{}:{}] [Recv warn(fd:{})] [Errmsg:{}]", __FILE__, __LINE__, this->m_fd, strerror(errno));
                m_error = strerror(errno);
            }
            return;
        }
        else if (len == 0)
        {
            if(m_success) m_success = false;
            m_error = "Connection Close";
            spdlog::info("[{}:{}] [Recv warn(fd:{})] [The peer closes the connection]", __FILE__, __LINE__, this->m_fd);
            this->m_scheduler.lock()->DelEvent(this->m_fd, poller::kEventError | poller::kEventRead | poller::kEventWrite);
            this->m_scheduler.lock()->DelObjector(this->m_fd);
            return;
        }
        else
        {
            this->m_rbuffer.append(buffer, len);
        }
    }
}

GY_TcpSendTask::GY_TcpSendTask(int fd, SSL* ssl, std::weak_ptr<poller::GY_IOScheduler> scheduler)
{
    this->m_fd = fd;
    this->m_ssl = ssl;
    this->m_scheduler = scheduler;
    this->m_success = false;
}

void 
GY_TcpSendTask::AppendWBuffer(std::string &&wbuffer)
{
    this->m_wbuffer.append(std::forward<std::string &&>(wbuffer));
}

void 
GY_TcpSendTask::SendAll()
{
    if (this->m_wbuffer.empty())
        return;
    Execute();
    if (this->m_wbuffer.empty())
        this->m_scheduler.lock()->DelEvent(this->m_fd, poller::kEventWrite);
}

bool 
GY_TcpSendTask::Empty() const
{
    return this->m_wbuffer.empty();
}

bool 
GY_TcpSendTask::Success()
{
    return m_success;
}

std::string 
GY_TcpSendTask::Error()
{
    return m_error;
}

void 
GY_TcpSendTask::Execute()
{
    spdlog::debug("[{}:{}] [Send(fd:{})] [Len:{} Package:{}]", __FILE__, __LINE__, this->m_fd, this->m_wbuffer.length(), this->m_wbuffer);
    int offset = 0;
    while (1)
    {
        if (this->m_wbuffer.size() == offset)
        {
            m_success = true;
            if(!m_error.empty()) m_error.clear();
            this->m_wbuffer.clear();
            return;
        }
        size_t len;
        if(!m_ssl){
            len = io::net::TcpFunction::Send(this->m_fd, this->m_wbuffer.data() + offset, this->m_wbuffer.length() - offset);
        }else{
            len = io::net::TcpFunction::SSLSend(this->m_ssl, this->m_wbuffer.data() + offset, this->m_wbuffer.length() - offset);
        }
        if (len == -1)
        {
            if(m_success) m_success = false;
            m_error = strerror(errno);
            if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
            {
                spdlog::error("[{}:{}] [Send error(fd:{})] [Errmsg:{}]", __FILE__, __LINE__, this->m_fd, strerror(errno));
                close(this->m_fd);
                this->m_scheduler.lock()->DelObjector(this->m_fd);
                this->m_scheduler.lock()->DelEvent(this->m_fd, poller::kEventError | poller::kEventRead | poller::kEventWrite);
            }
            else
            {
                spdlog::debug("[{}:{}] [[Send(fd:{})] [Errmsg:{}]]", __FILE__, __LINE__, this->m_fd, strerror(errno));
                this->m_wbuffer.erase(0, offset);
            }
            return;
        }
        else if (len == 0)
        {
            if(m_success) m_success = false;
            m_error = "Connection Close";
            spdlog::info("[{}:{}] [Send info(fd:{})] [The peer closes the connection]", __FILE__, __LINE__, this->m_fd);
            this->m_scheduler.lock()->DelObjector(this->m_fd);
            this->m_scheduler.lock()->DelEvent(this->m_fd, poller::kEventError | poller::kEventRead | poller::kEventWrite);
            return;
        }
        else
        {
            offset += len;
        }
    }
}


GY_TcpCreateSSLConnTask::GY_TcpCreateSSLConnTask(std::weak_ptr<server::GY_SIOManager> ioManager)
    : GY_TcpCreateConnTask(ioManager)
{
    auto sslConfig = ioManager.lock()->GetTcpServerBuilder().lock()->GetSSLConfig();
    long minVersion = sslConfig->GetMinSSLVersion();
    long maxVersion = sslConfig->GetMaxSSLVersion();
    this->m_sslCtx = io::net::TcpFunction::InitSSLServer(minVersion,maxVersion);
    if(!m_sslCtx) throw std::runtime_error("InitSSLServer failed");
    if(io::net::TcpFunction::SetSSLCertAndKey(m_sslCtx, sslConfig->GetCertPath().c_str(), sslConfig->GetKeyPath().c_str()) == -1){
        throw std::runtime_error("SetSSLCertAndKey failed");
    };
}

void 
GY_TcpCreateSSLConnTask::Execute()
{
    while(1)
    {
        if(CreateConn() == -1) break;
    }
}

int 
GY_TcpCreateSSLConnTask::CreateConn()
{
    int connfd = io::net::TcpFunction::Accept(this->m_fd);
    if (connfd == -1)
    {
        if (errno != EINTR && errno != EWOULDBLOCK && errno != ECONNABORTED && errno != EPROTO)
        {
            spdlog::error("[{}:{}] [[Accept error(fd:{})] [Errmsg:{}]]", __FILE__, __LINE__, this->m_fd, strerror(errno));
            if(m_success) m_success = false;
            m_error = strerror(errno);
        }
        else
        {
            spdlog::debug("[{}:{}] [[Accept warn(fd:{})] [Errmsg:{}]]", __FILE__, __LINE__, this->m_fd, strerror(errno));
            if(m_success) m_success = false;
            m_error = strerror(errno);
        }
        return -1;
    }
    spdlog::debug("[{}:{}] [[Accept success(fd:{})] [Fd:{}]]", __FILE__, __LINE__, this->m_fd, connfd);
    int ret = io::BlockFuction::SetNoBlock(connfd);
    if (ret == -1)
    {
        close(connfd);
        spdlog::error("[{}:{}] [[SetNoBlock error(fd:{})] [Errmsg:{}]]", __FILE__, __LINE__, connfd, strerror(errno));
        if(m_success) m_success = false;
        m_error = strerror(errno);
        return -1;
    }
    spdlog::debug("[{}:{}] [SetNoBlock Success(fd:{})]", __FILE__, __LINE__, connfd);
    SSL* ssl = io::net::TcpFunction::SSLCreateObj(m_sslCtx,connfd);
    std::shared_ptr<objector::GY_TcpConnector> connector = std::make_shared<objector::GY_TcpConnector>(connfd, ssl, this->m_ioManager);
    m_ioManager.lock()->GetIOScheduler()->RegisterObjector(connfd, connector);
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
            m_ioManager.lock()->GetIOScheduler()->DelObjector(connfd);
            close(connfd);
            if(m_success) m_success = false;
            m_error = "SSLAccept over times";
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
                if(m_success) m_success = false;
                m_error = msg;
                io::net::TcpFunction::SSLDestory(ssl);
                ssl = nullptr;
                spdlog::error("[{}:{}] [socket(fd: {}) close]", __FILE__, __LINE__, connfd);
                m_ioManager.lock()->GetIOScheduler()->DelObjector(connfd);
                close(connfd);
                return -1;
            }
        }
    }while (ret <= 0);
    spdlog::info("[{}:{}] [SSL_Accept success(fd:{})]", __FILE__, __LINE__, this->m_fd);
    if (m_ioManager.lock()->GetIOScheduler()->AddEvent(connfd, poller::kEventRead | poller::kEventEpollET | poller::kEventError) == -1)
    {
        close(connfd);
        if(m_success) m_success = false;
        m_error = strerror(errno);
        spdlog::error("[{}:{}] [AddEvent error(fd:{})] [Errmsg:{}]", __FILE__, __LINE__, connfd, strerror(errno));
        return -1;
    }
    spdlog::debug("[{}:{}] [AddEvent success(fd:{})]", __FILE__, __LINE__, connfd);
    return connfd;
}


GY_TcpCreateSSLConnTask::~GY_TcpCreateSSLConnTask()
{
    if(this->m_sslCtx) {
        io::net::TcpFunction::SSLDestory({},this->m_sslCtx);
        this->m_sslCtx = nullptr;
    }
}
}