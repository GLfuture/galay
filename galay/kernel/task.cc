#include "task.h"
#include <spdlog/spdlog.h>

galay::kernel::GY_CreateConnTask::GY_CreateConnTask(std::weak_ptr<GY_IOScheduler> scheduler)
{
    this->m_scheduler = scheduler;
    this->m_fd = CreateListenFd(this->m_scheduler.lock()->GetTcpServerBuilder());
    if(this->m_fd < 0) throw std::runtime_error("CreateListenFd error");
}

void galay::kernel::GY_CreateConnTask::Execute()
{
    while(1)
    {
        if(CreateConn() == -1) break;
    }
}

int 
galay::kernel::GY_CreateConnTask::CreateConn()
{
    int connfd = IOFunction::NetIOFunction::TcpFunction::Accept(this->m_fd);
    if (connfd == -1)
    {
        if (errno != EINTR && errno != EWOULDBLOCK && errno != ECONNABORTED && errno != EPROTO)
        {
            spdlog::error("[{}:{}] [[Accept error(fd:{})] [Errmsg:{}]]", __FILE__, __LINE__, this->m_fd, strerror(errno));
        }
        else
        {
            spdlog::debug("[{}:{}] [[Accept warn(fd:{})] [Errmsg:{}]]", __FILE__, __LINE__, this->m_fd, strerror(errno));
        }
        return -1;
    }
    spdlog::debug("[{}:{}] [[Accept success(fd:{})] [Fd:{}]]", __FILE__, __LINE__, this->m_fd, connfd);
    int ret = IOFunction::BlockFuction::IO_Set_No_Block(connfd);
    if (ret == -1)
    {
        close(connfd);
        spdlog::error("[{}:{}] [[IO_Set_No_Block error(fd:{})] [Errmsg:{}]]", __FILE__, __LINE__, this->m_fd, strerror(errno));
        return -1;
    }
    spdlog::debug("[{}:{}] [IO_Set_No_Block Success(fd:{})]", __FILE__, __LINE__, connfd);
    std::shared_ptr<GY_Connector> connector = std::make_shared<GY_Connector>(connfd, nullptr, this->m_scheduler);
    m_scheduler.lock()->RegisterObjector(connfd, connector);
    
    if (m_scheduler.lock()->AddEvent(connfd, EventType::kEventRead | EventType::kEvnetEpollET | EventType::kEventError) == -1)
    {
        close(connfd);
        spdlog::error("[{}:{}] [AddEvent error(fd:{})] [Errmsg:{}]", __FILE__, __LINE__, connfd, strerror(errno));
        return -1;
    }
    spdlog::debug("[{}:{}] [AddEvent success(fd:{})]", __FILE__, __LINE__, connfd);
    return connfd;
}

int 
galay::kernel::GY_CreateConnTask::CreateListenFd(std::weak_ptr<GY_TcpServerBuilderBase> builder)
{
    int fd = IOFunction::NetIOFunction::TcpFunction::Sock();
    if (fd <= 0)
    {
        spdlog::error("[{}:{}] [[Sock error] [Errmsg:{}]]", __FILE__, __LINE__, strerror(errno));
        return -1;
    }
    int ret = IOFunction::NetIOFunction::TcpFunction::ReuseAddr(fd);
    if (ret == -1)
    {
        spdlog::error("[{}:{}] [[ReuseAddr(fd: {}) error] [Errmsg:{}]]", __FILE__, __LINE__, fd, strerror(errno));
        close(fd);
        return -1;
    }
    ret = IOFunction::NetIOFunction::TcpFunction::ReusePort(fd);
    ret = IOFunction::NetIOFunction::TcpFunction::Bind(fd, builder.lock()->GetPort());
    if (ret == -1)
    {
        spdlog::error("[{}:{}] [[Bind(fd: {}) error] [Errmsg:{}]]", __FILE__, __LINE__, fd, strerror(errno));
        close(fd);
        return -1;
    }
    ret = IOFunction::NetIOFunction::TcpFunction::Listen(fd, builder.lock()->GetBacklog());
    if (ret == -1)
    {
        spdlog::error("[{}:{}] [[Listen(fd: {}) error] [Errmsg:{}]]", __FILE__, __LINE__, fd, strerror(errno));
        close(fd);
        return -1;
    }
    ret = IOFunction::NetIOFunction::TcpFunction::IO_Set_No_Block(fd);
    if (ret == -1)
    {
        spdlog::error("[{}:{}] [[IO_Set_No_Block(fd: {}) error] [Errmsg:{}]]", __FILE__, __LINE__, fd, strerror(errno));
        close(fd);
        return -1;
    }
    return fd;
}


int 
galay::kernel::GY_CreateConnTask::GetFd()
{
    return this->m_fd;
}

galay::kernel::GY_RecvTask::GY_RecvTask(int fd, SSL* ssl, std::weak_ptr<GY_IOScheduler> scheduler)
{
    this->m_fd = fd;
    this->m_ssl = ssl;
    this->m_scheduler = scheduler;
}


void 
galay::kernel::GY_RecvTask::RecvAll()
{
    Execute();
}

std::string&
galay::kernel::GY_RecvTask::GetRBuffer()
{
    return m_rbuffer;
}


void 
galay::kernel::GY_RecvTask::Execute()
{
    uint32_t BufferLen = m_scheduler.lock()->GetTcpServerBuilder().lock()->GetReadBufferLen();
    char buffer[BufferLen] = {0};
    while (1)
    {
        bzero(buffer, BufferLen);
        ssize_t len;
        if(!this->m_scheduler.lock()->GetTcpServerBuilder().lock()->GetIsSSL()){
            len = IOFunction::NetIOFunction::TcpFunction::Recv(this->m_fd, buffer, BufferLen);
        }else{
            len = IOFunction::NetIOFunction::TcpFunction::SSLRecv(this->m_ssl,buffer,BufferLen);
        }
        if (len == -1)
        {
            if (errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN)
            {
                spdlog::error("[{}:{}] [Fail(fd:{})] [Errmsg:{}]", __FILE__, __LINE__, this->m_fd, strerror(errno));
                close(this->m_fd);
                this->m_scheduler.lock()->DelObjector(this->m_fd);
                this->m_scheduler.lock()->DelEvent(this->m_fd, EventType::kEventError | EventType::kEventRead | EventType::kEventWrite);
            }
            else
            {
                spdlog::debug("[{}:{}] [[Recv warn(fd:{})] [Errmsg:{}]]", __FILE__, __LINE__, this->m_fd, strerror(errno));
            }
            return;
        }
        else if (len == 0)
        {
            spdlog::info("[{}:{}] [Recv warn(fd:{})] [The peer closes the connection]", __FILE__, __LINE__, this->m_fd);
            this->m_scheduler.lock()->DelEvent(this->m_fd, EventType::kEventError | EventType::kEventRead | EventType::kEventWrite);
            this->m_scheduler.lock()->DelObjector(this->m_fd);
            return;
        }
        else
        {
            spdlog::debug("[{}:{}] [Recv(fd:{})] [Recv len:{} Bytes]", __FILE__, __LINE__, this->m_fd, len);
            this->m_rbuffer.append(buffer, len);
        }
    }
}

galay::kernel::GY_SendTask::GY_SendTask(int fd, SSL* ssl, GY_IOScheduler::wptr scheduler)
{
    this->m_fd = fd;
    this->m_ssl = ssl;
    this->m_scheduler = scheduler;
}

void 
galay::kernel::GY_SendTask::AppendWBuffer(std::string &&wbuffer)
{
    this->m_wbuffer.append(std::forward<std::string &&>(wbuffer));
}

void 
galay::kernel::GY_SendTask::FirstTryToSend()
{
    if (this->m_wbuffer.empty())
        return;
    Execute();
    if (!this->m_wbuffer.empty())
    {
        this->m_scheduler.lock()->AddEvent(this->m_fd, EventType::kEventWrite);
    }
}

void 
galay::kernel::GY_SendTask::SendAll()
{
    if (this->m_wbuffer.empty())
        return;
    Execute();
    if (this->m_wbuffer.empty())
        this->m_scheduler.lock()->DelEvent(this->m_fd, EventType::kEventWrite);
}

bool 
galay::kernel::GY_SendTask::Empty() const
{
    return this->m_wbuffer.empty();
}

void 
galay::kernel::GY_SendTask::Execute()
{
    spdlog::info("[{}:{}] [Send(fd:{})] [Len:{} Package:{}]", __FILE__, __LINE__, this->m_fd, this->m_wbuffer.length(), this->m_wbuffer);
    int offset = 0;
    while (1)
    {
        if (this->m_wbuffer.size() == offset)
        {
            this->m_wbuffer.clear();
            return;
        }
        size_t len;
        if(!this->m_scheduler.lock()->GetTcpServerBuilder().lock()->GetIsSSL()){
            len = IOFunction::NetIOFunction::TcpFunction::Send(this->m_fd, this->m_wbuffer, this->m_wbuffer.length());
        }else{
            len = IOFunction::NetIOFunction::TcpFunction::SSLSend(this->m_ssl, this->m_wbuffer, this->m_wbuffer.length());
        }
        if (len == -1)
        {
            if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
            {
                spdlog::error("[{}:{}] [Send error(fd:{})] [Errmsg:{}]", __FILE__, __LINE__, this->m_fd, strerror(errno));
                close(this->m_fd);
                this->m_scheduler.lock()->DelObjector(this->m_fd);
                this->m_scheduler.lock()->DelEvent(this->m_fd, EventType::kEventError | EventType::kEventRead | EventType::kEventWrite);
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
            spdlog::info("[{}:{}] [Send info(fd:{})] [The peer closes the connection]", __FILE__, __LINE__, this->m_fd);
            this->m_scheduler.lock()->DelObjector(this->m_fd);
            this->m_scheduler.lock()->DelEvent(this->m_fd, EventType::kEventError | EventType::kEventRead | EventType::kEventWrite);
            return;
        }
        else
        {
            spdlog::debug("[{}:{}] [Send(fd:{})] [Send len:{} Bytes]", __FILE__, __LINE__, this->m_fd, len);
            offset += len;
        }
    }
}


galay::kernel::GY_CreateSSLConnTask::GY_CreateSSLConnTask(std::weak_ptr<GY_IOScheduler> scheduler)
    : GY_CreateConnTask(scheduler)
{
    auto sslConfig = scheduler.lock()->GetTcpServerBuilder().lock()->GetSSLConfig();
    long minVersion = sslConfig->GetMinSSLVersion();
    long maxVersion = sslConfig->GetMaxSSLVersion();
    this->m_sslCtx = IOFunction::NetIOFunction::TcpFunction::SSL_Init_Server(minVersion,maxVersion);
    if(!m_sslCtx) throw std::runtime_error("SSL_Init_Server failed");
    if(IOFunction::NetIOFunction::TcpFunction::SSL_Config_Cert_And_Key(m_sslCtx, sslConfig->GetCertPath().c_str(), sslConfig->GetKeyPath().c_str()) == -1){
        throw std::runtime_error("SSL_Config_Cert_And_Key failed");
    };
}

void 
galay::kernel::GY_CreateSSLConnTask::Execute()
{
    while(1)
    {
        if(CreateConn() == -1) break;
    }
}

int 
galay::kernel::GY_CreateSSLConnTask::CreateConn()
{
    int connfd = IOFunction::NetIOFunction::TcpFunction::Accept(this->m_fd);
    if (connfd == -1)
    {
        if (errno != EINTR && errno != EWOULDBLOCK && errno != ECONNABORTED && errno != EPROTO)
        {
            spdlog::error("[{}:{}] [[Accept error(fd:{})] [Errmsg:{}]]", __FILE__, __LINE__, this->m_fd, strerror(errno));
        }
        else
        {
            spdlog::debug("[{}:{}] [[Accept warn(fd:{})] [Errmsg:{}]]", __FILE__, __LINE__, this->m_fd, strerror(errno));
        }
        return -1;
    }
    spdlog::debug("[{}:{}] [[Accept success(fd:{})] [Fd:{}]]", __FILE__, __LINE__, this->m_fd, connfd);
    int ret = IOFunction::BlockFuction::IO_Set_No_Block(connfd);
    if (ret == -1)
    {
        close(connfd);
        spdlog::error("[{}:{}] [[IO_Set_No_Block error(fd:{})] [Errmsg:{}]]", __FILE__, __LINE__, connfd, strerror(errno));
        return -1;
    }
    spdlog::debug("[{}:{}] [IO_Set_No_Block Success(fd:{})]", __FILE__, __LINE__, connfd);
    SSL* ssl = IOFunction::NetIOFunction::TcpFunction::SSLCreateObj(m_sslCtx,connfd);
    std::shared_ptr<GY_Connector> connector = std::make_shared<GY_Connector>(connfd, ssl, this->m_scheduler);
    m_scheduler.lock()->RegisterObjector(connfd, connector);
    int retryTimes = 0;
    do{
        ret = IOFunction::NetIOFunction::TcpFunction::SSLAccept(ssl);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if(retryTimes++ > 4000){
            int ssl_err = SSL_get_error(ssl, ret);
            char msg[256];
            ERR_error_string(ssl_err, msg);
            spdlog::error("[{}:{}] [socket(fd: {}) SSL_Accept error: '{}']", __FILE__, __LINE__, connfd, msg);
            IOFunction::NetIOFunction::TcpFunction::SSLDestory(ssl);
            ssl = nullptr;
            spdlog::error("[{}:{}] [socket(fd: {}) close]", __FILE__, __LINE__, connfd);
            m_scheduler.lock()->DelObjector(connfd);
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
                IOFunction::NetIOFunction::TcpFunction::SSLDestory(ssl);
                ssl = nullptr;
                spdlog::error("[{}:{}] [socket(fd: {}) close]", __FILE__, __LINE__, connfd);
                m_scheduler.lock()->DelObjector(connfd);
                close(connfd);
                return -1;
            }
        }
    }while (ret <= 0);
    spdlog::info("[{}:{}] [SSL_Accept success(fd:{})]", __FILE__, __LINE__, this->m_fd);
     if (m_scheduler.lock()->AddEvent(connfd, EventType::kEventRead | EventType::kEvnetEpollET | EventType::kEventError) == -1)
    {
        close(connfd);
        spdlog::error("[{}:{}] [AddEvent error(fd:{})] [Errmsg:{}]", __FILE__, __LINE__, connfd, strerror(errno));
        return -1;
    }
    spdlog::debug("[{}:{}] [AddEvent success(fd:{})]", __FILE__, __LINE__, connfd);
    return connfd;
}


galay::kernel::GY_CreateSSLConnTask::~GY_CreateSSLConnTask()
{
    if(this->m_sslCtx) {
        IOFunction::NetIOFunction::TcpFunction::SSLDestory({},this->m_sslCtx);
        this->m_sslCtx = nullptr;
    }
}