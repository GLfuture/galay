#include "client.h"
#include "../kernel/callback.h"
#include <spdlog/spdlog.h>

void galay::Client::stop()
{
    if (!this->m_scheduler.expired() && !this->m_stop)
    {
        this->m_scheduler.lock()->del_task(this->m_fd);
        if(!Callback_ConnClose::empty()) Callback_ConnClose::call(this->m_fd);
        close(this->m_fd);
        this->m_scheduler.lock()->del_event(this->m_fd, GY_EVENT_READ | GY_EVENT_WRITE | GY_EVENT_ERROR);
        this->m_stop = true;
    }
}

galay::Client::~Client()
{
    if (!this->m_scheduler.expired() && !this->m_stop)
    {
        stop();
        this->m_stop = true;
    }
}

galay::Tcp_Client::Tcp_Client(Scheduler_Base::wptr scheduler)
    : Client(scheduler)
{
    if (!this->m_scheduler.expired())
    {
        this->m_fd = IOFuntion::TcpFunction::Sock();
        IOFuntion::BlockFuction::IO_Set_No_Block(this->m_fd);
    }
}

galay::Net_Awaiter galay::Tcp_Client::connect(std::string ip, uint32_t port)
{
    Co_Tcp_Client_Connect_Task::ptr task = std::make_shared<Co_Tcp_Client_Connect_Task>(this->m_fd, this->m_scheduler, &(this->m_error));
    int ret = IOFuntion::TcpFunction::Conncet(this->m_fd, ip, port);
    if (ret == 0)
    {
        this->m_error = Error::GY_SUCCESS;
        return Net_Awaiter{nullptr, ret};
    }
    else if (ret == -1)
    {
        if (errno != EINPROGRESS && errno != EINTR && errno != EWOULDBLOCK)
        {
            this->m_error = Error::GY_CONNECT_ERROR;
            return Net_Awaiter{nullptr, ret};
        }
    }
    if (!m_scheduler.expired())
    {
        this->m_scheduler.lock()->add_task({this->m_fd, task});
        if (this->m_scheduler.lock()->add_event(this->m_fd, GY_EVENT_WRITE) == -1)
        {
            spdlog::error("{} {} {} scheduler add fail(fd: {} ) {}, close connection",__TIME__,__FILE__,__LINE__,this->m_fd,strerror(errno));
        }else spdlog::info("{} {} {} scheduler add event success(fd: {})",__TIME__,__FILE__,__LINE__,this->m_fd);
    }
    return Net_Awaiter{task};
}

galay::Net_Awaiter galay::Tcp_Client::send(const std::string &buffer, uint32_t len)
{
    int ret = IOFuntion::TcpFunction::Send(this->m_fd, buffer, len);
    if (ret == 0)
    {
        this->m_error = Error::GY_SEND_ERROR;
        return Net_Awaiter{nullptr, -1};
    }
    else if (ret == -1)
    {
        if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
        {
            Co_Tcp_Client_Send_Task::ptr task = std::make_shared<Co_Tcp_Client_Send_Task>(this->m_fd, buffer, len, this->m_scheduler, &(this->m_error));
            if (!this->m_scheduler.expired())
            {
                this->m_scheduler.lock()->add_task({this->m_fd, task});
                if (this->m_scheduler.lock()->add_event(this->m_fd, GY_EVENT_WRITE) == -1)
                {
                    std::cout << "add event failed fd = " << this->m_fd << '\n';
                }
            }
            return Net_Awaiter{task};
        }
        else
        {
            this->m_error = Error::GY_SEND_ERROR;
            return Net_Awaiter{nullptr, ret};
        }
    }
    this->m_error = Error::GY_SUCCESS;
    return Net_Awaiter{nullptr, ret};
}

galay::Net_Awaiter galay::Tcp_Client::recv(char *buffer, int len)
{
    int ret = IOFuntion::TcpFunction::Recv(this->m_fd, buffer, len);
    if (ret == 0)
    {
        this->m_error = Error::GY_RECV_ERROR;
        return Net_Awaiter{nullptr, -1};
    }
    else if (ret == -1)
    {
        if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
        {
            Co_Tcp_Client_Recv_Task::ptr task = std::make_shared<Co_Tcp_Client_Recv_Task>(this->m_fd, buffer, len, this->m_scheduler, &(this->m_error));
            if (!this->m_scheduler.expired())
            {
                this->m_scheduler.lock()->add_task({this->m_fd, task});
                if (this->m_scheduler.lock()->add_event(this->m_fd, GY_EVENT_READ) == -1)
                {
                    std::cout << "add event failed fd = " << this->m_fd << '\n';
                }
            }
            return Net_Awaiter{task};
        }
        else
        {
            this->m_error = Error::GY_RECV_ERROR;
            return Net_Awaiter{nullptr, -1};
        }
    }
    this->m_error = Error::GY_SUCCESS;
    return Net_Awaiter{nullptr, ret};
}

galay::Tcp_Client::~Tcp_Client()
{

}

galay::Tcp_SSL_Client::Tcp_SSL_Client(Scheduler_Base::wptr scheduler, long ssl_min_version, long ssl_max_version)
    : Tcp_Client(scheduler)
{
    this->m_ctx = IOFuntion::TcpFunction::SSL_Init_Client(ssl_min_version, ssl_max_version);
    if (this->m_ctx == nullptr)
    {
        this->m_error = Error::NetError::GY_SSL_CTX_INIT_ERROR;
        ERR_print_errors_fp(stderr);
        exit(-1);
    }
    this->m_ssl = IOFuntion::TcpFunction::SSLCreateObj(this->m_ctx, this->m_fd);
    if (this->m_ssl == nullptr)
    {
        this->m_error = Error::NetError::GY_SSL_OBJ_INIT_ERROR;
        close(this->m_fd);
        exit(-1);
    }
}

galay::Net_Awaiter galay::Tcp_SSL_Client::connect(std::string ip, uint32_t port)
{
    int ret = IOFuntion::TcpFunction::Conncet(this->m_fd, ip, port);
    int status = Task_Status::GY_TASK_CONNECT;
    if (ret == 0)
    {
        status = Task_Status::GY_TASK_SSL_CONNECT;
    }
    else if (ret == -1)
    {
        if (errno != EINPROGRESS && errno != EINTR)
        {
            this->m_error = Error::GY_SSL_CONNECT_ERROR;
            return {nullptr, ret};
        }
    }
    Co_Tcp_Client_SSL_Connect_Task::ptr task = std::make_shared<Co_Tcp_Client_SSL_Connect_Task>(this->m_fd, this->m_ssl, this->m_scheduler, &(this->m_error), status);
    if (!m_scheduler.expired())
    {
        this->m_scheduler.lock()->add_task({this->m_fd, task});
        if (this->m_scheduler.lock()->add_event(this->m_fd, GY_EVENT_WRITE) == -1)
        {
            std::cout << "add event failed fd = " << this->m_fd << '\n';
        }
    }
    return Net_Awaiter{task};
}

galay::Net_Awaiter galay::Tcp_SSL_Client::send(const std::string &buffer, uint32_t len)
{
    int ret = IOFuntion::TcpFunction::SSLSend(this->m_ssl, buffer, buffer.length());
    if (ret == -1)
    {
        if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
        {
            auto task = std::make_shared<Co_Tcp_Client_SSL_Send_Task>(this->m_ssl, buffer, len, this->m_scheduler, &(this->m_error));
            if (!m_scheduler.expired())
            {
                this->m_scheduler.lock()->add_task({this->m_fd, task});
                if (this->m_scheduler.lock()->add_event(this->m_fd, GY_EVENT_WRITE) == -1)
                {
                    std::cout << "add event failed fd = " << this->m_fd << '\n';
                }
            }
            return {task};
        }
        else
        {
            this->m_error = Error::GY_SSL_SEND_ERROR;
            return {nullptr, ret};
        }
    }
    else if (ret == 0)
    {
        this->m_error = Error::GY_SSL_SEND_ERROR;
        return {nullptr, -1};
    }
    this->m_error = Error::GY_SUCCESS;
    return {nullptr, ret};
}

galay::Net_Awaiter galay::Tcp_SSL_Client::recv(char *buffer, int len)
{
    int ret = IOFuntion::TcpFunction::SSLRecv(this->m_ssl, buffer, len);
    if (ret == 0)
    {
        this->m_error = Error::GY_SSL_RECV_ERROR;
        return Net_Awaiter{nullptr, -1};
    }
    else if (ret == -1)
    {
        if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
        {
            auto task = std::make_shared<Co_Tcp_Client_SSL_Recv_Task>(this->m_ssl, buffer, len, this->m_scheduler, &(this->m_error));
            if (!this->m_scheduler.expired())
            {
                this->m_scheduler.lock()->add_task({this->m_fd, task});
                if (this->m_scheduler.lock()->add_event(this->m_fd, GY_EVENT_READ) == -1)
                {
                    std::cout << "add event failed fd = " << this->m_fd << '\n';
                }
            }
            return Net_Awaiter{task};
        }
        else
        {
            this->m_error = Error::GY_RECV_ERROR;
            return Net_Awaiter{nullptr, -1};
        }
    }
    this->m_error = Error::GY_SUCCESS;
    return Net_Awaiter{nullptr, ret};
}

galay::Tcp_SSL_Client::~Tcp_SSL_Client()
{
    if (m_ssl && m_ctx)
    {
        IOFuntion::TcpFunction::SSLDestory({this->m_ssl}, this->m_ctx);
    }
}

galay::Udp_Client::Udp_Client(Scheduler_Base::wptr scheduler)
    : Client(scheduler)
{
    if (!scheduler.expired())
    {
        this->m_fd = IOFuntion::UdpFunction::Sock();
        IOFuntion::BlockFuction::IO_Set_No_Block(this->m_fd);
    }
}

galay::Net_Awaiter galay::Udp_Client::sendto(std::string ip, uint32_t port, std::string buffer)
{
    if (!this->m_scheduler.expired())
    {
        auto task = std::make_shared<Co_Udp_Client_Sendto_Task>(this->m_fd, ip, port, buffer, this->m_scheduler, &(this->m_error));
        if (this->m_scheduler.lock()->add_event(this->m_fd, GY_EVENT_WRITE | GY_EVENT_EPOLLET | GY_EVENT_ERROR) == -1)
        {
            std::cout << "add event failed fd = " << this->m_fd << '\n';
        }
        this->m_scheduler.lock()->add_task({this->m_fd, task});
        return Net_Awaiter{task};
    }
    this->m_error = Error::SchedulerError::GY_SCHDULER_EXPIRED_ERROR;
    return {nullptr, -1};
}

galay::Net_Awaiter galay::Udp_Client::recvfrom(char *buffer, int len, IOFuntion::Addr *addr)
{
    if (!this->m_scheduler.expired())
    {
        auto task = std::make_shared<Co_Udp_Client_Recvfrom_Task>(this->m_fd, addr, buffer, len, this->m_scheduler, &(this->m_error));
        if (this->m_scheduler.lock()->add_event(this->m_fd, GY_EVENT_READ | GY_EVENT_EPOLLET | GY_EVENT_ERROR) == -1)
        {
            std::cout << "add event failed fd = " << this->m_fd << '\n';
        }
        this->m_scheduler.lock()->add_task({this->m_fd, task});
        return Net_Awaiter{task};
    }
    this->m_error = Error::SchedulerError::GY_SCHDULER_EXPIRED_ERROR;
    return {nullptr, -1};
}