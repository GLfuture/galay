#include "task.h"

//tcp rw task
galay::Tcp_RW_Task::Tcp_RW_Task(int fd, std::weak_ptr<Epoll_Scheduler> scheduler, uint32_t read_len)
{
    this->m_status = Task_Status::GY_TASK_READ;
    this->m_error = error::base_error::GY_SUCCESS;
    this->m_scheduler = scheduler;
    this->m_fd = fd;
    this->m_read_len = read_len;
    this->m_temp = new char[read_len];
    this->m_req = std::make_shared<Tcp_Request>();
    this->m_resp = std::make_shared<Tcp_Response>();
}


void galay::Tcp_RW_Task::control_task_behavior(Task_Status status)
{
    if (!this->m_scheduler.expired())
    {
        auto scheduler = this->m_scheduler.lock();
        switch (status)
        {
        case Task_Status::GY_TASK_WRITE:
        {
            scheduler->mod_event(this->m_fd, EPOLLOUT);
            this->m_status = Task_Status::GY_TASK_WRITE;
            break;
        }
        case Task_Status::GY_TASK_READ:
        {
            scheduler->mod_event(this->m_fd, EPOLLIN);
            this->m_status = Task_Status::GY_TASK_READ;
            break;
        }
        case Task_Status::GY_TASK_DISCONNECT:
        {
            this->m_status = Task_Status::GY_TASK_DISCONNECT;
            break;
        }
        default:
            break;
        }
    }
}

int galay::Tcp_RW_Task::exec()
{
    switch (this->m_status)
    {
    case Task_Status::GY_TASK_READ:
    {
        if (read_package() == -1)
            return -1;
        if (decode() == error::protocol_error::GY_PROTOCOL_INCOMPLETE)
            return -1;
        control_task_behavior(Task_Status::GY_TASK_WRITE);
        m_co_task = this->m_func(this->shared_from_this());
        break;
    }
    case Task_Status::GY_TASK_WRITE:
    {
        if (this->m_is_finish)
        {
            encode();
            if (send_package() == -1)
                return -1;
            control_task_behavior(Task_Status::GY_TASK_READ);
            this->m_is_finish = false;
        }
        break;
    }
    default:
        break;
    }
    return 0;
}

galay::Epoll_Scheduler::ptr galay::Tcp_RW_Task::get_scheduler()
{
    if (!this->m_scheduler.expired())
        return this->m_scheduler.lock();
    return nullptr;
}

void galay::Tcp_RW_Task::reset_buffer(int len)
{
    if (this->m_temp)
        delete[] this->m_temp;
    this->m_temp = new char[len];
    this->m_read_len = len;
}

bool galay::Tcp_RW_Task::is_need_to_destroy()
{
    return this->m_is_finish && this->m_wbuffer.empty() && this->m_status == Task_Status::GY_TASK_DISCONNECT;
}

int galay::Tcp_RW_Task::decode()
{
    int state = error::base_error::GY_SUCCESS;
    int len = this->m_req->decode(this->m_rbuffer, state);
    if (state == error::protocol_error::GY_PROTOCOL_INCOMPLETE)
        return state;
    this->m_rbuffer.erase(this->m_rbuffer.begin(), this->m_rbuffer.begin() + len);
    return state;
}

void galay::Tcp_RW_Task::encode()
{
    m_wbuffer.append(m_resp->encode());
}

int galay::Tcp_RW_Task::read_package()
{
    this->m_error = error::base_error::GY_SUCCESS;
    int len = iofunction::Tcp_Function::Recv(this->m_fd, this->m_temp, this->m_read_len);
    if (len == 0)
    {
        finish();
        control_task_behavior(GY_TASK_DISCONNECT);
        return -1;
    }
    else if (len == -1)
    {
        if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
        {
            return -1;
        }
        else
        {
            finish();
            control_task_behavior(GY_TASK_DISCONNECT);
            return -1;
        }
    }
    this->m_rbuffer.append(this->m_temp, len);
    memset(this->m_temp, 0, len);
    return 0;
}

int galay::Tcp_RW_Task::send_package()
{
    int len = iofunction::Tcp_Function::Send(this->m_fd, this->m_wbuffer, this->m_wbuffer.length());
    if (len == -1)
    {
        if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
        {
            return -1;
        }
        else
        {
            finish();
            control_task_behavior(GY_TASK_DISCONNECT);
            return -1;
        }
    }
    else if (len == 0 && !this->m_wbuffer.empty())
    {
        finish();
        control_task_behavior(GY_TASK_DISCONNECT);
        return -1;
    }
    this->m_wbuffer.erase(this->m_wbuffer.begin(), this->m_wbuffer.begin() + len);
    return 0;
}


galay::Tcp_RW_Task::~Tcp_RW_Task()
{
    if (m_temp)
    {
        delete[] m_temp;
        m_temp = nullptr;
    }
}


//tcp accept task
galay::Tcp_Accept_Task::Tcp_Accept_Task(int fd, std::weak_ptr<Epoll_Scheduler> scheduler,
                                        std::function<Task<>(Task_Base::wptr)> &&func, uint32_t read_len)
{
    this->m_fd = fd;
    this->m_status = Task_Status::GY_TASK_READ;
    this->m_scheduler = scheduler;
    this->m_func = func;
    this->m_read_len = read_len;
}

int galay::Tcp_Accept_Task::exec()
{
    int connfd = iofunction::Tcp_Function::Accept(this->m_fd);
    if (connfd <= 0)
        return -1;
    if (this->m_is_keepalive)
    {
        int ret = iofunction::Tcp_Function::Sock_Keepalive(connfd,this->m_idle,this->m_interval,this->m_retry);
        if(ret == -1){
            std::cout<<strerror(errno)<<'\n';
        }
    }
    if (!this->m_scheduler.expired())
    {
        auto task = create_rw_task(connfd);
        this->m_scheduler.lock()->add_task({connfd,task});
        iofunction::Tcp_Function::IO_Set_No_Block(connfd);
        this->m_scheduler.lock()->add_event(connfd, EPOLLIN | EPOLLET);
    }
    return 0;
}

bool galay::Tcp_Accept_Task::is_need_to_destroy()
{
    return this->m_is_finish;
}

void galay::Tcp_Accept_Task::enable_keepalive(uint16_t idle , uint16_t interval,uint16_t retry)
{
    this->m_is_keepalive = true;
    this->m_idle = idle;
    this->m_interval = interval;
    this->m_retry = retry;
}

galay::Task_Base::ptr galay::Tcp_Accept_Task::create_rw_task(int connfd)
{
    auto task = std::make_shared<Tcp_RW_Task>(connfd, this->m_scheduler, this->m_read_len);
    task->set_callback(std::forward<std::function<Task<>(Task_Base::wptr)>>(this->m_func));
    return task;
}

galay::Tcp_Accept_Task::~Tcp_Accept_Task()
{

}

//tcp ssl rw task
galay::Tcp_SSL_RW_Task::Tcp_SSL_RW_Task(int fd, std::weak_ptr<Epoll_Scheduler> scheduler, uint32_t read_len, SSL *ssl)
    : Tcp_RW_Task(fd, scheduler, read_len), m_ssl(ssl)
{
}


int galay::Tcp_SSL_RW_Task::read_package()
{
    int len = iofunction::Tcp_Function::SSL_Recv(this->m_ssl, this->m_temp, this->m_read_len);
    if (len == 0)
    {
        finish();
        control_task_behavior(GY_TASK_DISCONNECT);
        return -1;
    }
    else if (len == -1)
    {
        if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
        {
            return -1;
        }
        else
        {
            finish();
            control_task_behavior(GY_TASK_DISCONNECT);
            return -1;
        }
    }
    this->m_rbuffer.append(this->m_temp, len);
    memset(this->m_temp, 0, len);
    return 0;
}

int galay::Tcp_SSL_RW_Task::send_package()
{
    int len = iofunction::Tcp_Function::SSL_Send(this->m_ssl, this->m_wbuffer, this->m_wbuffer.length());
    if (len == -1)
    {
        if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
        {
            return -1;
        }
        else
        {
            finish();
            control_task_behavior(GY_TASK_DISCONNECT);
            return -1;
        }
    }
    else if (len == 0 && !this->m_wbuffer.empty())
    {
        finish();
        control_task_behavior(GY_TASK_DISCONNECT);
        return -1;
    }
    this->m_wbuffer.erase(this->m_wbuffer.begin(), this->m_wbuffer.begin() + len);
    return 0;
}

galay::Tcp_SSL_RW_Task::~Tcp_SSL_RW_Task()
{
    if (this->m_ssl)
    {
        iofunction::Tcp_Function::SSL_Destory(this->m_ssl);
        this->m_ssl = nullptr;
    }
}

//tcp ssl accept task
galay::Tcp_SSL_Accept_Task::Tcp_SSL_Accept_Task(int fd, std::weak_ptr<Epoll_Scheduler> scheduler, std::function<Task<>(Task_Base::wptr)> &&func,
                                                uint32_t read_len, uint32_t ssl_accept_max_retry, SSL_CTX *ctx)
    : Tcp_Accept_Task(fd, scheduler, std::forward<std::function<Task<>(Task_Base::wptr)>>(func), read_len),
      m_ctx(ctx), m_ssl_accept_retry(ssl_accept_max_retry) 
{

}

int galay::Tcp_SSL_Accept_Task::exec()
{
    int connfd = iofunction::Tcp_Function::Accept(this->m_fd);
    if (connfd <= 0)
        return -1;
    if (this->m_is_keepalive)
    {
        iofunction::Tcp_Function::Sock_Keepalive(connfd,this->m_idle,this->m_interval,this->m_retry);
    }
    SSL *ssl = iofunction::Tcp_Function::SSL_Create_Obj(this->m_ctx, connfd);
    if (ssl == nullptr)
    {
        close(connfd);
        return -1;
    }
    iofunction::Tcp_Function::IO_Set_No_Block(connfd);
    int ret = iofunction::Tcp_Function::SSL_Accept(ssl);
    uint32_t retry = 0;
    while (ret <= 0 && retry++ <= this->m_ssl_accept_retry)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_SSL_SLEEP_MISC_PER_RETRY));
        ret = iofunction::Tcp_Function::SSL_Accept(ssl);
        if (ret <= 0)
        {
            int ssl_err = SSL_get_error(ssl, ret);
            if (ssl_err == SSL_ERROR_WANT_READ || ssl_err == SSL_ERROR_WANT_WRITE || ssl_err == SSL_ERROR_WANT_ACCEPT)
            {
                continue;
            }
            else
            {
                iofunction::Tcp_Function::SSL_Destory(ssl);
                close(connfd);
                return -1;
            }
        }
    }

    if (!this->m_scheduler.expired())
    {
        auto task = create_rw_task(connfd, ssl);
        this->m_scheduler.lock()->add_task({connfd,task});
        this->m_scheduler.lock()->add_event(connfd, EPOLLIN | EPOLLET);
    }
    return 0;
}

galay::Task_Base::ptr galay::Tcp_SSL_Accept_Task::create_rw_task(int connfd, SSL *ssl)
{
    auto task = std::make_shared<Tcp_SSL_RW_Task>(connfd, this->m_scheduler, this->m_read_len, ssl);
    task->set_callback(std::forward<std::function<Task<>(Task_Base::wptr)>>(this->m_func));
    return task;
}

galay::Tcp_SSL_Accept_Task::~Tcp_SSL_Accept_Task()
{
    if (this->m_ctx)
        this->m_ctx = nullptr;
}

//http rw task
galay::Http_RW_Task::Http_RW_Task(int fd, std::weak_ptr<Epoll_Scheduler> scheduler, uint32_t read_len)
    : Tcp_RW_Task(fd, scheduler, read_len)
{
    this->m_req = std::make_shared<Http_Request>();
    this->m_resp = std::make_shared<Http_Response>();
}

bool galay::Http_RW_Task::is_need_to_destroy()
{
    return Tcp_RW_Task::is_need_to_destroy();
}

int galay::Http_RW_Task::exec()
{
    switch (this->m_status)
    {
    case Task_Status::GY_TASK_READ:
    {
        if (Tcp_RW_Task::read_package() == -1)
            return -1;
        if (Tcp_RW_Task::decode() == error::protocol_error::GY_PROTOCOL_INCOMPLETE)
            return -1;
        Tcp_RW_Task::control_task_behavior(Task_Status::GY_TASK_WRITE);
        this->m_co_task = this->m_func(this->shared_from_this());
        break;
    }
    case Task_Status::GY_TASK_WRITE:
    {
        if (this->m_is_finish)
        {
            if (send_head)
            {
                Tcp_RW_Task::encode();
                send_head = false;
            }
            if (Tcp_RW_Task::send_package() == -1)
                return -1;
            if (!this->m_wbuffer.empty())
                return -1;
            control_task_behavior(GY_TASK_DISCONNECT);
        }
        break;
    }
    default:
        break;
    }
    return 0;
}

galay::Http_RW_Task::~Http_RW_Task()
{

}

//http accept task
galay::Http_Accept_Task::Http_Accept_Task(int fd, std::weak_ptr<Epoll_Scheduler> scheduler, std::function<Task<>(Task_Base::wptr)> &&func, uint32_t read_len)
    : Tcp_Accept_Task(fd, scheduler, std::forward<std::function<Task<>(Task_Base::wptr)>>(func), read_len)
{
}

galay::Task_Base::ptr galay::Http_Accept_Task::create_rw_task(int connfd)
{
    if (!this->m_scheduler.expired())
    {
        auto task = std::make_shared<Http_RW_Task>(connfd, this->m_scheduler, this->m_read_len);
        task->set_callback(std::forward<std::function<Task<>(Task_Base::wptr)>>(this->m_func));
        return task;
    }
    return nullptr;
}

galay::Http_Accept_Task::~Http_Accept_Task()
{

}


//https rw task
galay::Https_RW_Task::Https_RW_Task(int fd, std::weak_ptr<Epoll_Scheduler> scheduler, uint32_t read_len, SSL *ssl)
    : Tcp_SSL_RW_Task(fd, scheduler, read_len, ssl)
{
    this->m_req = std::make_shared<Http_Request>();
    this->m_resp = std::make_shared<Http_Response>();
}

int galay::Https_RW_Task::exec()
{
    switch (this->m_status)
    {
    case Task_Status::GY_TASK_READ:
    {
        if (Tcp_SSL_RW_Task::read_package() == -1)
            return -1;
        if (Tcp_SSL_RW_Task::decode() == error::protocol_error::GY_PROTOCOL_INCOMPLETE)
            return -1;
        Tcp_SSL_RW_Task::control_task_behavior(Task_Status::GY_TASK_WRITE);
        this->m_co_task = this->m_func(this->shared_from_this());
        break;
    }
    case Task_Status::GY_TASK_WRITE:
    {
        if (this->m_is_finish)
        {
            if (send_head)
            {
                Tcp_SSL_RW_Task::encode();
                send_head = false;
            }
            if (Tcp_SSL_RW_Task::send_package() == -1)
                return -1;
            if (!this->m_wbuffer.empty())
                return -1;
            control_task_behavior(GY_TASK_DISCONNECT);
        }

        break;
    }
    default:
        break;
    }
    return 0;
}

galay::Https_RW_Task::~Https_RW_Task()
{

}

//https accept task
galay::Https_Accept_Task::Https_Accept_Task(int fd, std::weak_ptr<Epoll_Scheduler> scheduler, std::function<Task<>(Task_Base::wptr)> &&func
    , uint32_t read_len, uint32_t ssl_accept_max_retry, SSL_CTX *ctx)
    : Tcp_SSL_Accept_Task(fd, scheduler, std::forward<std::function<Task<>(Task_Base::wptr)>>(func), read_len, ssl_accept_max_retry, ctx) 
{}

galay::Task_Base::ptr galay::Https_Accept_Task::create_rw_task(int connfd, SSL *ssl)
{
    auto task = std::make_shared<Https_RW_Task>(connfd, this->m_scheduler, this->m_read_len, ssl);
    task->set_callback(std::forward<std::function<Task<>(Task_Base::wptr)>>(this->m_func));
    return task;
}

galay::Https_Accept_Task::~Https_Accept_Task()
{

}

//time task
galay::Time_Task::Time_Task(std::weak_ptr<Timer_Manager> manager)
{
    this->m_manager = manager;
}

int galay::Time_Task::exec()
{
    if (!m_manager.expired())
    {
        auto timer = m_manager.lock()->get_ealist_timer();
        if(timer) timer->exec();
    }
    return 0;
}

bool galay::Time_Task::is_need_to_destroy()
{
    return this->m_is_finish;
}

galay::Time_Task::~Time_Task()
{

}

//thread task
galay::Thread_Task::Thread_Task(std::function<void()>&& func)
{
    this->m_func = func;
}

int galay::Thread_Task::exec()
{
    this->m_func();
    return 0;
}

bool galay::Thread_Task::is_need_to_destroy()
{
    return this->m_is_finish;
}

galay::Thread_Task::~Thread_Task()
{

}