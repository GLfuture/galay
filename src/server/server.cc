#include "server.h"

galay::Server::~Server()
{
    if (!this->m_scheduler->is_stop())
    {
        this->m_scheduler->stop();
    }
}

void galay::Tcp_Server::start(std::function<Task<>(Task_Base::wptr)> &&func)
{
    Tcp_Server_Config::ptr config = std::dynamic_pointer_cast<Tcp_Server_Config>(this->m_config);
    this->m_error = init(config);
    if (this->m_error != error::base_error::GY_SUCCESS)
        return;
    add_accept_task(std::forward<std::function<Task<>(Task_Base::wptr)>>(func), config->m_max_rbuffer_len);
    this->m_error = this->m_scheduler->start();
}

int galay::Tcp_Server::init(Tcp_Server_Config::ptr config)
{
    this->m_fd = iofunction::Tcp_Function::Sock();
    if (this->m_fd <= 0)
    {
        return error::base_error::GY_SOCKET_ERROR;
    }
    int ret = iofunction::Tcp_Function::Reuse_Fd(this->m_fd);
    if (ret == -1)
    {
        close(this->m_fd);
        return error::server_error::GY_SETSOCKOPT_ERROR;
    }
    ret = iofunction::Tcp_Function::Bind(this->m_fd, config->m_port);
    if (ret == -1)
    {
        close(this->m_fd);
        return error::server_error::GY_BIND_ERROR;
    }
    ret = iofunction::Tcp_Function::Listen(this->m_fd, config->m_backlog);
    if (ret == -1)
    {
        close(this->m_fd);
        return error::server_error::GY_LISTEN_ERROR;
    }
    iofunction::Tcp_Function::IO_Set_No_Block(this->m_fd);
    return error::base_error::GY_SUCCESS;
}

void galay::Tcp_Server::add_accept_task(std::function<Task<>(Task_Base::wptr)> &&func, uint32_t recv_len)
{
    this->m_scheduler->m_engine->add_event(this->m_fd, EPOLLIN | EPOLLET);
    this->m_scheduler->m_tasks->emplace(std::make_pair(this->m_fd, std::make_shared<Tcp_Accept_Task>(this->m_fd, this->m_scheduler, std::forward<std::function<Task<>(Task_Base::wptr)>>(func), recv_len)));
}

galay::Tcp_SSL_Server::Tcp_SSL_Server(Tcp_SSL_Server_Config::ptr config, IO_Scheduler::ptr scheduler)
    : Tcp_Server(config, scheduler)
{
    m_ctx = iofunction::Tcp_Function::SSL_Init_Server(config->m_ssl_min_version, config->m_ssl_max_version);
    if (m_ctx == nullptr)
    {
        this->m_error = error::server_error::GY_SSL_CTX_INIT_ERROR;
        ERR_print_errors_fp(stderr);
        exit(-1);
    }
    if (iofunction::Tcp_Function::SSL_Config_Cert_And_Key(m_ctx, config->m_cert_filepath.c_str(), config->m_key_filepath.c_str()) == -1)
    {
        this->m_error = error::server_error::GY_SSL_CRT_OR_KEY_FILE_ERROR;
        ERR_print_errors_fp(stderr);
        exit(-1);
    }
}

galay::Tcp_SSL_Server::~Tcp_SSL_Server()
{
    if (this->m_ctx)
    {
        iofunction::Tcp_Function::SSL_Destory({}, m_ctx);
        this->m_ctx = nullptr;
    }
}

void galay::Tcp_SSL_Server::add_accept_task(std::function<Task<>(Task_Base::wptr)> &&func, uint32_t recv_len)
{
    this->m_scheduler->m_engine->add_event(this->m_fd, EPOLLIN | EPOLLET);
    Tcp_SSL_Server_Config::ptr config = std::dynamic_pointer_cast<Tcp_SSL_Server_Config>(this->m_config);
    this->m_scheduler->m_tasks->emplace(std::make_pair(this->m_fd, std::make_shared<Tcp_SSL_Accept_Task>(this->m_fd, this->m_scheduler, std::forward<std::function<Task<>(Task_Base::wptr)>>(func), recv_len, config->m_ssl_accept_retry, this->m_ctx)));
}

void galay::Http_Server::add_accept_task(std::function<Task<>(Task_Base::wptr)> &&func, uint32_t max_recv_len)
{
    this->m_scheduler->m_engine->add_event(this->m_fd, EPOLLIN | EPOLLET);
    this->m_scheduler->m_tasks->emplace(std::make_pair(this->m_fd, std::make_shared<Http_Accept_Task>(this->m_fd, this->m_scheduler, std::forward<std::function<Task<>(Task_Base::wptr)>>(func), max_recv_len)));
}

void galay::Https_Server::add_accept_task(std::function<Task<>(Task_Base::wptr)> &&func, uint32_t max_recv_len)
{
    this->m_scheduler->m_engine->add_event(this->m_fd, EPOLLIN | EPOLLET);
    Https_Server_Config::ptr config = std::dynamic_pointer_cast<Https_Server_Config>(this->m_config);
    this->m_scheduler->m_tasks->emplace(std::make_pair(this->m_fd, std::make_shared<Https_Accept_Task>(this->m_fd, this->m_scheduler, std::forward<std::function<Task<>(Task_Base::wptr)>>(func), max_recv_len, config->m_ssl_accept_retry, this->m_ctx)));
}