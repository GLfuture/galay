#include "co_task.h"
#include <spdlog/spdlog.h>
 
void 
galay::Co_Task_Base::set_co_handle(std::coroutine_handle<> handle)
{
    this->m_handle = handle;
}

std::any
galay::Co_Task_Base::result()
{
    return this->m_result;
}

galay::Co_Task_Base::~Co_Task_Base()
{
    if (m_handle)
    {
        m_handle = nullptr;
    }
}

galay::Co_Tcp_Client_Connect_Task::Co_Tcp_Client_Connect_Task(int fd, Scheduler_Base::wptr scheduler, int *error)
{
    this->m_fd = fd;
    this->m_error = error;
    this->m_scheduler = scheduler;
}

int 
galay::Co_Tcp_Client_Connect_Task::exec()
{
    int status = 0;
    socklen_t slen = sizeof(status);
    if (getsockopt(this->m_fd, SOL_SOCKET, SO_ERROR, (void *)&status, &slen) < 0)
    {
        *(this->m_error) = Error::GY_CONNECT_ERROR;
        spdlog::error("[{}:{}] [getsocketopt(fd: {}) error: {}]", __FILE__, __LINE__, this->m_fd, strerror(errno));
        this->m_result = -1;
    }
    if (status != 0)
    {
        *(this->m_error) = Error::GY_CONNECT_ERROR;
        spdlog::error("[{}:{}] [connect(fd: {}) error: {}]", __FILE__, __LINE__, this->m_fd, strerror(errno));
        this->m_result = -1;
    }
    else
    {
        spdlog::info("[{}:{}] [connect(fd: {}) success]", __FILE__, __LINE__, this->m_fd);
        *(this->m_error) = Error::GY_SUCCESS;
        this->m_result = 0;
    }
    if (!this->m_handle.done())
    {
        this->m_scheduler.lock()->del_task(this->m_fd);
        this->m_scheduler.lock()->del_event(this->m_fd, GY_EVENT_READ | GY_EVENT_WRITE | GY_EVENT_ERROR);
        this->m_handle.resume();
    }
    return 0;
}

galay::Co_Tcp_Client_Send_Task::Co_Tcp_Client_Send_Task(int fd, const std::string &buffer, uint32_t len, Scheduler_Base::wptr scheduler, int *error)
{
    this->m_fd = fd;
    this->m_buffer = buffer;
    this->m_len = len;
    this->m_error = error;
    this->m_scheduler = scheduler;
}

int 
galay::Co_Tcp_Client_Send_Task::exec()
{
    int ret = IOFuntion::TcpFunction::Send(this->m_fd, this->m_buffer, this->m_len);
    if (ret == -1)
    {
        if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
        {
            spdlog::warn("[{}:{}] [Send(fd: {}) warn: {}]", __FILE__, __LINE__, this->m_fd, strerror(errno));
            return -1;
        }
        else
        {
            spdlog::error("[{}:{}] [Send(fd: {}) error: {}]", __FILE__, __LINE__, this->m_fd, strerror(errno));
            *(this->m_error) = Error::GY_SEND_ERROR;
            this->m_result = -1;
        }
    }
    else if (ret == 0)
    {
        spdlog::error("[{}:{}] [Send(fd: {}) error: {}]", __FILE__, __LINE__, this->m_fd, strerror(errno));
        *(this->m_error) = Error::GY_SEND_ERROR;
        this->m_result = -1;
    }
    else
    {
        spdlog::info("[{}:{}] [Send(fd :{}): {} Bytes]", __FILE__, __LINE__, this->m_fd, ret);
        spdlog::info("[{}:{}] [Send(fd: {}) Success]", __FILE__, __LINE__, this->m_fd);
        this->m_result = ret;
        *(this->m_error) = Error::GY_SUCCESS;
    }
    if (!this->m_handle.done())
    {
        this->m_scheduler.lock()->del_task(this->m_fd);
        this->m_scheduler.lock()->del_event(this->m_fd, GY_EVENT_READ | GY_EVENT_WRITE | GY_EVENT_ERROR);
        this->m_handle.resume();
    }
    return 0;
}

galay::Co_Tcp_Client_Recv_Task::Co_Tcp_Client_Recv_Task(int fd, char *buffer, int len, Scheduler_Base::wptr scheduler, int *error)
{
    this->m_fd = fd;
    this->m_buffer = buffer;
    this->m_len = len;
    this->m_error = error;
    this->m_scheduler = scheduler;
}

int 
galay::Co_Tcp_Client_Recv_Task::exec()
{
    int ret = IOFuntion::TcpFunction::Recv(this->m_fd, this->m_buffer, this->m_len);
    if (ret == -1)
    {
        if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
        {
            spdlog::warn("[{}:{}] [Recv(fd: {}) warn: {}]", __FILE__, __LINE__, this->m_fd, strerror(errno));
            return -1;
        }
        else
        {
            spdlog::error("[{}:{}] [Recv(fd: {}) error: {}", __FILE__, __LINE__, this->m_fd, strerror(errno));
            *(this->m_error) = Error::GY_RECV_ERROR;
            this->m_result = -1;
        }
    }
    else if (ret == 0)
    {
        spdlog::error("[{}:{}] [Recv(fd: {}) error: {}]", __FILE__, __LINE__, this->m_fd, strerror(errno));
        *(this->m_error) = Error::GY_RECV_ERROR;
        this->m_result = -1;
    }
    else
    {
        spdlog::info("[{}:{}] [SSL_Send(fd :{}): {} Bytes]", __FILE__, __LINE__, this->m_fd, ret);
        spdlog::info("[{}:{}] [Recv(fd: {}) Success]", __FILE__, __LINE__, this->m_fd);
        *(this->m_error) = Error::GY_SUCCESS;
        this->m_result = ret;
    }
    if (!this->m_handle.done())
    {
        this->m_scheduler.lock()->del_task(this->m_fd);
        this->m_scheduler.lock()->del_event(this->m_fd, GY_EVENT_READ | GY_EVENT_WRITE | GY_EVENT_ERROR);
        this->m_handle.resume();
    }
    return 0;
}

galay::Co_Tcp_Client_SSL_Connect_Task::Co_Tcp_Client_SSL_Connect_Task(int fd, SSL *ssl, Scheduler_Base::wptr scheduler, int *error, int init_status)
{
    this->m_fd = fd;
    this->m_error = error;
    this->m_status = init_status;
    this->m_ssl = ssl;
    this->m_scheduler = scheduler;
}

int 
galay::Co_Tcp_Client_SSL_Connect_Task::exec()
{

    switch (this->m_status)
    {
    case Task_Status::GY_TASK_CONNECT:
    {
        int status = 0;
        socklen_t slen = sizeof(status);
        if (getsockopt(this->m_fd, SOL_SOCKET, SO_ERROR, (void *)&status, &slen) < 0)
        {
            spdlog::error("[{}:{}] [getsocketopt(fd: {}) error: {}]", __FILE__, __LINE__, this->m_fd, strerror(errno));
            *(this->m_error) = Error::GY_CONNECT_ERROR;
            this->m_result = -1;
        }
        if (status != 0)
        {
            spdlog::error("[{}:{}] [connect(fd: {}) error: {}]", __FILE__, __LINE__, this->m_fd, strerror(errno));
            *(this->m_error) = Error::GY_CONNECT_ERROR;
            this->m_result = -1;
        }
        else
        {
            spdlog::info("[{}:{}] [connect(fd : {}) Success]", __FILE__, __LINE__, this->m_fd);
            this->m_status = Task_Status::GY_TASK_SSL_CONNECT;
            return -1;
        }
        break;
    }
    case Task_Status::GY_TASK_SSL_CONNECT:
    {
        int ret = IOFuntion::TcpFunction::SSLConnect(this->m_ssl);
        if (ret <= 0)
        {
            int status = SSL_get_error(this->m_ssl, ret);
            if (status == SSL_ERROR_WANT_READ || status == SSL_ERROR_WANT_WRITE || status == SSL_ERROR_WANT_CONNECT)
            {
                spdlog::warn("[{}:{}] [SSL_connect(fd: {}) warn: {}]", __FILE__, __LINE__, this->m_fd, strerror(errno));
                return -1;
            }
            else
            {
                spdlog::error("[{}:{}] [SSL_connect(fd: {}) error: {}]", __FILE__, __LINE__, this->m_fd, strerror(errno));
                *(this->m_error) = Error::GY_SSL_CONNECT_ERROR;
                this->m_result = -1;
            }
        }
        else
        {
            spdlog::info("[{}:{}] [SSL_connect(fd : {}) Success]", __FILE__, __LINE__, this->m_fd);
            *(this->m_error) = Error::GY_SUCCESS;
            this->m_result = 0;
        }
        break;
    }
    }

    if (!this->m_handle.done())
    {
        this->m_scheduler.lock()->del_task(this->m_fd);
        this->m_scheduler.lock()->del_event(this->m_fd, GY_EVENT_READ | GY_EVENT_WRITE | GY_EVENT_ERROR);
        this->m_handle.resume();
    }
    return 0;
}

galay::Co_Tcp_Client_SSL_Send_Task::Co_Tcp_Client_SSL_Send_Task(SSL *ssl, const std::string &buffer, uint32_t len, Scheduler_Base::wptr scheduler, int *error)
{
    this->m_ssl = ssl;
    this->m_buffer = buffer;
    this->m_len = len;
    this->m_error = error;
    this->m_scheduler = scheduler;
}

int 
galay::Co_Tcp_Client_SSL_Send_Task::exec()
{
    int ret = IOFuntion::TcpFunction::SSLSend(this->m_ssl, this->m_buffer, this->m_len);
    if (ret == -1)
    {
        if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
        {
            spdlog::warn("[{}:{}] [SSL_Send(fd: {}) warn: {}]", __FILE__, __LINE__, SSL_get_fd(this->m_ssl), strerror(errno));
            return -1;
        }
        else
        {
            spdlog::error("[{}:{}] [SSL_Send(fd: {}) error: {}]", __FILE__, __LINE__, SSL_get_fd(this->m_ssl), strerror(errno));
            *(this->m_error) = Error::GY_SSL_SEND_ERROR;
            this->m_result = -1;
        }
    }
    else if (ret == 0)
    {
        spdlog::error("[{}:{}] [SSL_Send(fd: {}) error: {}]", __FILE__, __LINE__, SSL_get_fd(this->m_ssl), strerror(errno));
        *(this->m_error) = Error::GY_SSL_SEND_ERROR;
        this->m_result = -1;
    }
    else
    {
        spdlog::info("[{}:{}] [SSL_Send(fd :{}): {} Bytes]", __FILE__, __LINE__, SSL_get_fd(this->m_ssl), ret);
        spdlog::info("[{}:{}] [SSL_Send(fd: {}) Success]", __FILE__, __LINE__, SSL_get_fd(this->m_ssl));
        this->m_result = ret;
        *(this->m_error) = Error::GY_SUCCESS;
    }
    if (!this->m_handle.done())
    {
        this->m_scheduler.lock()->del_task(SSL_get_fd(this->m_ssl));
        this->m_scheduler.lock()->del_event(SSL_get_fd(this->m_ssl), GY_EVENT_READ | GY_EVENT_WRITE | GY_EVENT_ERROR);
        this->m_handle.resume();
    }
    return 0;
}

galay::Co_Tcp_Client_SSL_Recv_Task::Co_Tcp_Client_SSL_Recv_Task(SSL *ssl, char *buffer, int len, Scheduler_Base::wptr scheduler, int *error)
{
    this->m_ssl = ssl;
    this->m_buffer = buffer;
    this->m_len = len;
    this->m_error = error;
    this->m_scheduler = scheduler;
}

int 
galay::Co_Tcp_Client_SSL_Recv_Task::exec()
{
    int ret = IOFuntion::TcpFunction::SSLRecv(this->m_ssl, this->m_buffer, this->m_len);
    if (ret == -1)
    {
        if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
        {
            spdlog::warn("[{}:{}] [SSL_Recv(fd: {}) warn: {}]", __FILE__, __LINE__, SSL_get_fd(this->m_ssl), strerror(errno));
            return -1;
        }
        else
        {
            spdlog::error("[{}:{}] [SSL_Recv(fd: {}) error: {}]", __FILE__, __LINE__, SSL_get_fd(this->m_ssl), strerror(errno));
            *(this->m_error) = Error::GY_SSL_RECV_ERROR;
            this->m_result = -1;
        }
    }
    else if (ret == 0)
    {
        spdlog::error("[{}:{}] [SSL_Recv(fd: {}) error: {}]", __FILE__, __LINE__, SSL_get_fd(this->m_ssl), strerror(errno));
        *(this->m_error) = Error::GY_SSL_RECV_ERROR;
        this->m_result = -1;
    }
    else
    {
        spdlog::info("[{}:{}] [SSL_Recv(fd :{}): {} Bytes]", __FILE__, __LINE__, SSL_get_fd(this->m_ssl), ret);
        spdlog::info("[{}:{}] [SSL_Recv(fd: {}) Succcess]", __FILE__, __LINE__, SSL_get_fd(this->m_ssl));
        *(this->m_error) = Error::GY_SUCCESS;
        this->m_result = ret;
    }
    if (!this->m_handle.done())
    {
        this->m_scheduler.lock()->del_task(SSL_get_fd(this->m_ssl));
        this->m_scheduler.lock()->del_event(SSL_get_fd(this->m_ssl), GY_EVENT_READ | GY_EVENT_WRITE | GY_EVENT_ERROR);
        this->m_handle.resume();
    }
    return 0;
}

galay::Co_Udp_Client_Sendto_Task::Co_Udp_Client_Sendto_Task(int fd, std::string ip, uint32_t port, std::string buffer, Scheduler_Base::wptr scheduler, int *error)
{
    this->m_error = error;
    this->m_fd = fd;
    this->m_buffer = buffer;
    this->m_ip = ip,
    this->m_port = port;
    this->m_scheduler = scheduler;
}

int galay::Co_Udp_Client_Sendto_Task::exec()
{

    int ret = IOFuntion::UdpFunction::SendTo(this->m_fd, {m_ip, static_cast<int>(m_port)}, m_buffer);
    if (ret == -1)
    {
        if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
        {
            spdlog::warn("[{}:{}] [Sendto(fd: {}) warn: {}]", __FILE__, __LINE__, this->m_fd, strerror(errno));
            return -1;
        }
        else
        {
            spdlog::error("[{}:{}] [Sendto(fd: {}) error: {}]", __FILE__, __LINE__, this->m_fd, strerror(errno));
            *(this->m_error) = Error::GY_SENDTO_ERROR;
            this->m_result = -1;
        }
    }
    else if (ret == 0)
    {
        spdlog::error("[{}:{}] [Sendto(fd: {}) error: {}]", __FILE__, __LINE__, this->m_fd, strerror(errno));
        *(this->m_error) = Error::GY_SENDTO_ERROR;
        this->m_result = -1;
    }
    else
    {
        spdlog::info("[{}:{}] [Sendto(fd :{}): {} Bytes]", __FILE__, __LINE__, this->m_fd, ret);
        spdlog::info("[{}:{}] [Sendto(fd: {}) Success]", __FILE__, __LINE__, this->m_fd);
        *(this->m_error) = Error::GY_SUCCESS;
        this->m_result = ret;
    }

    if (!this->m_handle.done())
    {
        this->m_scheduler.lock()->del_task(this->m_fd);
        this->m_scheduler.lock()->del_event(this->m_fd, GY_EVENT_READ | GY_EVENT_WRITE | GY_EVENT_ERROR);
        this->m_handle.resume();
    }
    return 0;
}

galay::Co_Udp_Client_Recvfrom_Task::Co_Udp_Client_Recvfrom_Task(int fd, IOFuntion::Addr *addr, char *buffer, int len, Scheduler_Base::wptr scheduler, int *error)
{
    this->m_scheduler = scheduler;
    this->m_fd = fd;
    this->m_addr = addr;
    this->m_buffer = buffer;
    this->m_len = len;
    this->m_error = error;
    this->m_timer = scheduler.lock()->get_timer_manager()->add_timer(MAX_UDP_WAIT_FOR_RECV_TIME, 1, [this]()
                                                                     {
                if (!this->m_handle.done()) {
                    this->m_result = -1;
                    this->m_scheduler.lock()->del_event(this->m_fd,GY_EVENT_READ | GY_EVENT_WRITE| GY_EVENT_ERROR);
                    this->m_scheduler.lock()->del_task(this->m_fd);
                    this->m_handle.resume();
                } });
}

int galay::Co_Udp_Client_Recvfrom_Task::exec()
{
    int ret = IOFuntion::UdpFunction::RecvFrom(this->m_fd, *m_addr, m_buffer, m_len);
    if (ret == -1)
    {
        if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
        {
            spdlog::warn("[{}:{}] [Recvfrom(fd: {}) warn: {}]", __FILE__, __LINE__, this->m_fd, strerror(errno));
            return -1;
        }
        else
        {
            spdlog::error("[{}:{}] [Recvfrom(fd: {}) error: {}]", __FILE__, __LINE__, this->m_fd, strerror(errno));
            *(this->m_error) = Error::GY_SENDTO_ERROR;
            this->m_result = -1;
        }
    }
    else if (ret == 0)
    {
        spdlog::error("[{}:{}] [Recvfrom(fd: {}) error: {}]", __FILE__, __LINE__, this->m_fd, strerror(errno));
        *(this->m_error) = Error::GY_SENDTO_ERROR;
        this->m_result = -1;
    }
    else
    {
        spdlog::info("[{}:{}] [Recvfrom(fd :{}): {} Bytes]", __FILE__, __LINE__, this->m_fd, ret);
        spdlog::info("[{}:{}] [Recvfrom(fd: {}) Success]", __FILE__, __LINE__, this->m_fd);
        *(this->m_error) = Error::GY_SUCCESS;
        this->m_result = ret;
    }

    if (!this->m_handle.done())
    {
        this->m_timer->cancle();
        this->m_scheduler.lock()->del_task(this->m_fd);
        this->m_scheduler.lock()->del_event(this->m_fd, GY_EVENT_READ | GY_EVENT_WRITE | GY_EVENT_ERROR);
        this->m_handle.resume();
    }
    return 0;
}