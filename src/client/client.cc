#include "client.h"

void galay::Client::add_task(Task_Base::ptr task)
{
    if (!m_scheduler.expired())
    {
        auto scheduler = m_scheduler.lock();
        auto it = scheduler->m_tasks->find(this->m_fd);
        if (it == scheduler->m_tasks->end())
        {
            scheduler->m_tasks->emplace(std::make_pair(this->m_fd, task));
        }
        else
        {
            it->second = task;
        }
    }
}

galay::Tcp_Client::Tcp_Client(IO_Scheduler::wptr scheduler)
    : Client(scheduler)
{
    if (!this->m_scheduler.expired())
    {
        this->m_fd = iofunction::Tcp_Function::Sock();
        iofunction::Simple_Fuction::IO_Set_No_Block(this->m_fd);
        this->m_scheduler.lock()->m_engine->add_event(this->m_fd, EPOLLET | EPOLLIN);
    }
}

galay::Net_Awaiter<int> galay::Tcp_Client::connect(std::string ip, uint32_t port)
{
    typename Co_Tcp_Client_Connect_Task<int>::ptr task = std::make_shared<Co_Tcp_Client_Connect_Task<int>>(this->m_fd);

    int ret = iofunction::Tcp_Function::Conncet(this->m_fd, ip, port);
    if (ret == 0)
    {
        return Net_Awaiter<int>{nullptr, ret};
    }
    else if (ret == -1)
    {
        if (errno != EINPROGRESS && errno != EINTR)
        {
            return Net_Awaiter<int>{nullptr, ret};
        }
    }
    Client::add_task(task);
    if (!m_scheduler.expired())
    {
        this->m_scheduler.lock()->m_engine->mod_event(this->m_fd, EPOLLOUT);
    }
    return Net_Awaiter<int>{task};
}

galay::Net_Awaiter<int> galay::Tcp_Client::send(const std::string &buffer, uint32_t len)
{
    int ret = iofunction::Tcp_Function::Send(this->m_fd, buffer, len);
    if (ret == 0)
    {
        return Net_Awaiter<int>{nullptr, -1};
    }
    else if (ret == -1)
    {
        if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
        {
            typename Co_Tcp_Client_Send_Task<int>::ptr task = std::make_shared<Co_Tcp_Client_Send_Task<int>>(this->m_fd, buffer, len);
            Client::add_task(task);

            if (!this->m_scheduler.expired())
                this->m_scheduler.lock()->m_engine->mod_event(this->m_fd, EPOLLOUT);
            return Net_Awaiter<int>{task};
        }
        else
        {
            return Net_Awaiter<int>{nullptr, ret};
        }
    }
    return Net_Awaiter<int>{nullptr, ret};
}

galay::Net_Awaiter<int> galay::Tcp_Client::recv(char *buffer, int len)
{
    int ret = iofunction::Tcp_Function::Recv(this->m_fd, buffer, len);
    if (ret == 0)
    {
        return Net_Awaiter<int>{nullptr, -1};
    }
    else if (ret == -1)
    {
        if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
        {
            typename Co_Tcp_Client_Recv_Task<int>::ptr task = std::make_shared<Co_Tcp_Client_Recv_Task<int>>(this->m_fd, buffer, len);
            Client::add_task(task);
            if (!this->m_scheduler.expired())
                this->m_scheduler.lock()->m_engine->mod_event(this->m_fd, EPOLLIN);
            return Net_Awaiter<int>{task};
        }
        else
        {
            return Net_Awaiter<int>{nullptr, -1};
        }
    }
    return Net_Awaiter<int>{nullptr, ret};
}

void galay::Tcp_Client::disconnect()
{
    if (!this->m_scheduler.expired())
    {
        this->m_scheduler.lock()->m_tasks->at(this->m_fd)->finish();
    }
}

galay::Tcp_SSL_Client::Tcp_SSL_Client(IO_Scheduler::ptr scheduler, long ssl_min_version, long ssl_max_version, uint32_t ssl_connect_max_retry)
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

galay::Net_Awaiter<int> galay::Http_Client::request(Http_Request::ptr request, Http_Response::ptr response)
{
    if (!this->m_scheduler.expired())
    {
        typename Http_Request_Task<int>::ptr task = std::make_shared<Http_Request_Task<int>>(this->m_fd, this->m_scheduler.lock()->m_engine, request, response);
        Tcp_Client::add_task(task);
        return Net_Awaiter<int>{task};
    }
    return {nullptr, -1};
}