#include "client.h"

galay::Tcp_Client::Tcp_Client(Scheduler_Base::wptr scheduler)
    : Client(scheduler)
{
    if (!this->m_scheduler.expired())
    {
        this->m_fd = iofunction::Tcp_Function::Sock();
        iofunction::Simple_Fuction::IO_Set_No_Block(this->m_fd);
        this->m_scheduler.lock()->add_event(this->m_fd, GY_EVENT_READ);
    }
}

galay::Net_Awaiter<int> galay::Tcp_Client::connect(std::string ip, uint32_t port)
{
    typename Co_Tcp_Client_Connect_Task<int>::ptr task = std::make_shared<Co_Tcp_Client_Connect_Task<int>>(this->m_fd, &(this->m_error));

    int ret = iofunction::Tcp_Function::Conncet(this->m_fd, ip, port);
    if (ret == 0)
    {
        this->m_error = error::GY_SUCCESS;
        return Net_Awaiter<int>{nullptr, ret};
    }
    else if (ret == -1)
    {
        if (errno != EINPROGRESS && errno != EINTR)
        {
            this->m_error = error::GY_CONNECT_ERROR;
            return Net_Awaiter<int>{nullptr, ret};
        }
    }
    if (!m_scheduler.expired())
    {
        this->m_scheduler.lock()->add_task({this->m_fd,task});
        this->m_scheduler.lock()->mod_event(this->m_fd, GY_EVENT_WRITE);
    }
    return Net_Awaiter<int>{task};
}

galay::Net_Awaiter<int> galay::Tcp_Client::send(const std::string &buffer, uint32_t len)
{
    int ret = iofunction::Tcp_Function::Send(this->m_fd, buffer, len);
    if (ret == 0)
    {
        this->m_error = error::GY_SEND_ERROR;
        return Net_Awaiter<int>{nullptr, -1};
    }
    else if (ret == -1)
    {
        if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
        {
            typename Co_Tcp_Client_Send_Task<int>::ptr task = std::make_shared<Co_Tcp_Client_Send_Task<int>>(this->m_fd, buffer, len , &(this->m_error));
            if (!this->m_scheduler.expired())
            {
                this->m_scheduler.lock()->add_task({this->m_fd,task});
                this->m_scheduler.lock()->mod_event(this->m_fd, GY_EVENT_WRITE);
            }
            return Net_Awaiter<int>{task};
        }
        else
        {
            this->m_error = error::GY_SEND_ERROR;
            return Net_Awaiter<int>{nullptr, ret};
        }
    }
    this->m_error = error::GY_SUCCESS;
    return Net_Awaiter<int>{nullptr, ret};
}

galay::Net_Awaiter<int> galay::Tcp_Client::recv(char *buffer, int len)
{
    int ret = iofunction::Tcp_Function::Recv(this->m_fd, buffer, len);
    if (ret == 0)
    {
        this->m_error = error::GY_RECV_ERROR;
        return Net_Awaiter<int>{nullptr, -1};
    }
    else if (ret == -1)
    {
        if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
        {
            typename Co_Tcp_Client_Recv_Task<int>::ptr task = std::make_shared<Co_Tcp_Client_Recv_Task<int>>(this->m_fd, buffer, len , &(this->m_error));
            if (!this->m_scheduler.expired())
            {
                this->m_scheduler.lock()->add_task({this->m_fd,task});
                this->m_scheduler.lock()->mod_event(this->m_fd, GY_EVENT_READ);
            }
            return Net_Awaiter<int>{task};
        }
        else
        {
            this->m_error = error::GY_RECV_ERROR;
            return Net_Awaiter<int>{nullptr, -1};
        }
    }
    this->m_error = error::GY_SUCCESS;
    return Net_Awaiter<int>{nullptr, ret};
}


galay::Tcp_SSL_Client::Tcp_SSL_Client(Scheduler_Base::wptr scheduler, long ssl_min_version, long ssl_max_version)
    : Tcp_Client(scheduler)
{
    this->m_ctx = iofunction::Tcp_Function::SSL_Init_Client(ssl_min_version, ssl_max_version);
    if (this->m_ctx == nullptr)
    {
        this->m_error = error::server_error::GY_SSL_CTX_INIT_ERROR;
        ERR_print_errors_fp(stderr);
        exit(-1);
    }
    this->m_ssl = iofunction::Tcp_Function::SSL_Create_Obj(this->m_ctx, this->m_fd);
    if (this->m_ssl == nullptr)
    {
        close(this->m_fd);
        exit(-1);
    }
}


galay::Net_Awaiter<int> galay::Tcp_SSL_Client::connect(std::string ip , uint32_t port)
{
    int ret = iofunction::Tcp_Function::Conncet(this->m_fd, ip, port);
    int status = Task_Status::GY_TASK_CONNECT;
    if (ret == 0)
    {
        status = Task_Status::GY_TASK_SSL_CONNECT;
    }
    else if (ret == -1)
    {
        if (errno != EINPROGRESS && errno != EINTR)
        {
            this->m_error = error::GY_SSL_CONNECT_ERROR;
            return {nullptr,ret};
        }
    }
    typename Co_Tcp_Client_SSL_Connect_Task<int>::ptr task = std::make_shared<Co_Tcp_Client_SSL_Connect_Task<int>>(this->m_fd, this->m_ssl , &(this->m_error),status);
    if (!m_scheduler.expired())
    {
        this->m_scheduler.lock()->add_task({this->m_fd,task});
        this->m_scheduler.lock()->mod_event(this->m_fd, GY_EVENT_WRITE);
    }
    return Net_Awaiter<int>{task};
}

galay::Net_Awaiter<int> galay::Tcp_SSL_Client::send(const std::string &buffer,uint32_t len)
{
    int ret = iofunction::Tcp_Function::SSL_Send(this->m_ssl, buffer, buffer.length());
    if (ret == -1)
    {
        if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
        {
            auto task = std::make_shared<Co_Tcp_Client_SSL_Send_Task<int>>(this->m_ssl,buffer,len,&(this->m_error));
            if (!m_scheduler.expired())
            {
                this->m_scheduler.lock()->add_task({this->m_fd,task});
                this->m_scheduler.lock()->mod_event(this->m_fd, GY_EVENT_WRITE);
            }
            return {task};
        }else{
            this->m_error = error::GY_SSL_SEND_ERROR;
            return {nullptr,ret};
        }
    }
    else if (ret == 0)
    {
        this->m_error = error::GY_SSL_SEND_ERROR;
        return {nullptr,-1};
    }
    this->m_error = error::GY_SUCCESS;
    return {nullptr,ret};
}


galay::Net_Awaiter<int> galay::Tcp_SSL_Client::recv(char* buffer,int len)
{
    int ret = iofunction::Tcp_Function::SSL_Recv(this->m_ssl, buffer, len);
    if (ret == 0)
    {
        this->m_error = error::GY_SSL_RECV_ERROR;
        return Net_Awaiter<int>{nullptr, -1};
    }
    else if (ret == -1)
    {
        if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
        {
            auto task = std::make_shared<Co_Tcp_Client_SSL_Recv_Task<int>>(this->m_ssl, buffer, len , &(this->m_error));
            if (!this->m_scheduler.expired())
            {
                this->m_scheduler.lock()->add_task({this->m_fd,task});
                this->m_scheduler.lock()->mod_event(this->m_fd, GY_EVENT_READ);
            }
            return Net_Awaiter<int>{task};
        }
        else
        {
            this->m_error = error::GY_RECV_ERROR;
            return Net_Awaiter<int>{nullptr, -1};
        }
    }
    this->m_error = error::GY_SUCCESS;
    return Net_Awaiter<int>{nullptr, ret};
}


galay::Tcp_SSL_Client::~Tcp_SSL_Client()
{
    if (m_ssl && m_ctx)
    {
        iofunction::Tcp_Function::SSL_Destory({this->m_ssl}, this->m_ctx);
    }
}

galay::Net_Awaiter<int> galay::Http_Client::request(Http_Request::ptr request, Http_Response::ptr response)
{
    if (!this->m_scheduler.expired())
    {
        typename Http_Request_Task<int>::ptr task = std::make_shared<Http_Request_Task<int>>(this->m_fd, this->m_scheduler, request, response , &(this->m_error));
        this->m_scheduler.lock()->add_event(this->m_fd,GY_EVENT_WRITE);
        this->m_scheduler.lock()->add_task({this->m_fd,task});
        return Net_Awaiter<int>{task};
    }
    this->m_error = error::scheduler_error::GY_SCHDULER_IS_EXPIRED;
    return {nullptr, -1};
}


galay::Net_Awaiter<int> galay::Https_Client::request(Http_Request::ptr request,Http_Response::ptr response)
{
    if (!this->m_scheduler.expired())
    {
        auto task = std::make_shared<Https_Request_Task<int>>(this->m_ssl, this->m_fd ,  this->m_scheduler, request, response , &(this->m_error));
        this->m_scheduler.lock()->add_event(this->m_fd,GY_EVENT_WRITE);
        this->m_scheduler.lock()->add_task({this->m_fd,task});
        return Net_Awaiter<int>{task};
    }
    this->m_error = error::scheduler_error::GY_SCHDULER_IS_EXPIRED;
    return {nullptr, -1};
}