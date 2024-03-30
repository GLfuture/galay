#ifndef GALAY_TASK_H
#define GALAY_TASK_H

#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <chrono>
#include <algorithm>
#include <spdlog/spdlog.h>
#include "base.h"
#include "coroutine.h"
#include "iofunction.h"
#include "scheduler.h"
#include "callback.h"


namespace galay
{

    class Timer_Manager;


    template <typename RESULT = void>
    class Task : public Coroutine<RESULT>
    {
    public:
        using promise_type = Promise<RESULT>;
        Task()
        {
        }

        Task(std::coroutine_handle<promise_type> co_handle) noexcept
            : Coroutine<RESULT>(co_handle)
        {
        }

        Task(Task<RESULT> &&other) noexcept
            : Coroutine<RESULT>(other)
        {
        }

        Task<RESULT> &operator=(const Task<RESULT> &other) = delete;

        Task<RESULT> &operator=(Task<RESULT> &&other)
        {
            this->m_handle = other.m_handle;
            other.m_handle = nullptr;
            return *this;
        }
    };

    // tcp
    // server read and write task
    template <Tcp_Request REQ, Tcp_Response RESP>
    class Tcp_RW_Task : public Task_Base, public std::enable_shared_from_this<Tcp_RW_Task<REQ,RESP>>
    {
    protected:
        using Callback = std::function<Task<>(Task_Base::wptr)>;

    public:
        using ptr = std::shared_ptr<Tcp_RW_Task>;
        Tcp_RW_Task(int fd, std::weak_ptr<Scheduler_Base> scheduler, uint32_t read_len, int conn_timeout)
        {
            this->m_status = Task_Status::GY_TASK_READ;
            this->m_error = Error::NoError::GY_SUCCESS;
            this->m_scheduler = scheduler;
            this->m_fd = fd;
            this->m_read_len = read_len;
            this->m_temp = new char[read_len];
            this->m_req = std::make_shared<REQ>();
            this->m_resp = std::make_shared<RESP>();
            this->m_conn_timeout = conn_timeout;
            if(conn_timeout > 0){
                add_timeout_timer();
            }
        }

        void control_task_behavior(Task_Status status) override
        {
            if (!this->m_scheduler.expired())
            {
                auto scheduler = this->m_scheduler.lock();
                switch (status)
                {
                case Task_Status::GY_TASK_WRITE:
                {
                    if(scheduler->mod_event(this->m_fd, GY_EVENT_READ , GY_EVENT_WRITE) == -1)
                    {
                        spdlog::error("{} {} {} {}",__TIME__,__FILE__,__LINE__,Error::get_err_str(this->m_error));
                    }
                    this->m_status = Task_Status::GY_TASK_WRITE;
                    break;
                }
                case Task_Status::GY_TASK_READ:
                {
                    if(scheduler->mod_event(this->m_fd, GY_EVENT_WRITE , GY_EVENT_READ)==-1)
                    {
                        spdlog::error("{} {} {} {}",__TIME__,__FILE__,__LINE__,Error::get_err_str(this->m_error));
                    }
                    this->m_status = Task_Status::GY_TASK_READ;
                    break;
                }
                default:
                    break;
                }
            }
        }
        Tcp_Request_Base::ptr get_req() override { return this->m_req; }
        Tcp_Response_Base::ptr get_resp() override { return this->m_resp; }

        // return -1 to delete this task from server
        int exec() override
        {
            if (this->m_conn_timeout > 0)
            {
                this->m_timer->cancle();
                add_timeout_timer();
            }
            switch (this->m_status)
            {
            case Task_Status::GY_TASK_READ:
            {
                if (read_package() == -1)
                    return -1;
                m_co_task = this->m_func(this->shared_from_this());
                break;
            }
            case Task_Status::GY_TASK_WRITE:
            {

                encode();
                if (send_package() == -1)
                    return -1;
                if (!this->m_is_finish)
                    control_task_behavior(Task_Status::GY_TASK_READ);
                else
                    Task_Base::destory();
                break;
            }
            default:
                break;
            }
            return 0;
        }

        std::shared_ptr<Scheduler_Base> get_scheduler() override
        {
            if (!this->m_scheduler.expired())
                return this->m_scheduler.lock();
            return nullptr;
        }

        std::string &Get_Rbuffer() { return m_rbuffer; }

        std::string &Get_Wbuffer() { return m_wbuffer; }

        void set_callback(Callback &&func) { this->m_func = func; }

        void reset_buffer(int len)
        {
            if (this->m_temp)
                delete[] this->m_temp;
            this->m_temp = new char[len];
            this->m_read_len = len;
        }

        virtual int get_error() { return this->m_error; }

        virtual ~Tcp_RW_Task()
        {
            if (m_temp)
            {
                delete[] m_temp;
                m_temp = nullptr;
            }
            if (m_timer)
            {
                if (!m_timer->is_cancled())
                {
                    m_timer->cancle();
                }
            }
        }

    protected:
        void add_timeout_timer()
        {
            this->m_timer = this->m_scheduler.lock()->get_timer_manager()->add_timer(this->m_conn_timeout, 1, [this]() {
                    if(!Callback_ConnClose::empty()) Callback_ConnClose::call(this->m_fd);
                    close(this->m_fd);
                    this->m_scheduler.lock()->del_event(this->m_fd,GY_EVENT_READ|GY_EVENT_WRITE|GY_EVENT_ERROR);
                    this->m_scheduler.lock()->del_task(this->m_fd);
                });
        }

        void encode()
        {
            m_wbuffer.append(m_resp->encode());
        }

        virtual int read_package()
        {
            if(read_all_buffer() == -1) return -1;
            this->m_error = Error::NoError::GY_SUCCESS;
            switch (m_req->proto_type())
            {
                //buffer read empty
            case GY_ALL_RECIEVE_PROTOCOL_TYPE:
                return get_all_buffer();
                //fixed proto package
            case GY_PACKAGE_FIXED_PROTOCOL_TYPE:
                return get_fixed_package();
                //fixed head and head includes body's length
            case GY_HEAD_FIXED_PROTOCOL_TYPE:
                return get_fixed_head_package();
            }
            return -1;
        }

        virtual int send_package()
        {
            int len;
            if(this->m_wbuffer.empty()) {
                spdlog::warn("{} {} {} Send(fd: {}) {} Bytes",__TIME__,__FILE__,__LINE__,this->m_fd,0);
                return 0;
            }
            do{
                len = IOFuntion::TcpFunction::Send(this->m_fd, this->m_wbuffer, this->m_wbuffer.length());
                if (len != -1 && len != 0) {
                    spdlog::info("{} {} {} Send(fd: {}) {} Bytes",__TIME__,__FILE__,__LINE__,this->m_fd,len);
                    this->m_wbuffer.erase(this->m_wbuffer.begin(), this->m_wbuffer.begin() + len);
                }
                if(this->m_wbuffer.empty()) break;
            }while(len != 0 && len != -1);
            if (len == -1)
            {
                if (errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN)
                {
                    spdlog::error("{} {} {} Send(fd: {}) occurs error: {}, close connection",__TIME__,__FILE__,__LINE__,this->m_fd,strerror(errno));
                    Task_Base::destory();
                    return -1;
                }
            }
            else if (len == 0)
            {
                spdlog::error("{} {} {} SSL_Send (fd: {}) occurs error: {}, close connection",__TIME__,__FILE__,__LINE__,this->m_fd,strerror(errno));
                Task_Base::destory();
                return -1;
            } 
            return 0;
        }

        int read_all_buffer()
        {
            int len;
            do
            {
                memset(this->m_temp, 0, this->m_read_len);
                len = IOFuntion::TcpFunction::Recv(this->m_fd, this->m_temp, this->m_read_len);
                if (len != -1 && len != 0){
                    spdlog::info("{} {} {} Recv(fd: {}) {} Bytes",__TIME__,__FILE__,__LINE__,this->m_fd,len);
                    this->m_rbuffer.append(this->m_temp, len);
                }
            } while (len != -1 && len != 0);
            if (len == -1)
            {
                if (errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN)
                {
                    spdlog::error("{} {} {} Recv(fd: {}) occurs error: {}, close connection",__TIME__,__FILE__,__LINE__,this->m_fd,strerror(errno));
                    Task_Base::destory();
                    return -1;
                }
            }
            else if (len == 0)
            {
                spdlog::error("{} {} {} Recv(fd: {}) The peer closes the connection, error: {}, close connection",__TIME__,__FILE__,__LINE__,this->m_fd,strerror(errno));
                Task_Base::destory();
                return -1;
            }
            return 0;
        }

        int get_all_buffer()
        {
            int len = this->m_req->decode(this->m_rbuffer, this->m_error);
            spdlog::info("{} {} {} decode protocol (fd: {}) len: {}",__TIME__,__FILE__,__LINE__,this->m_fd,len);
            if (this->m_error == Error::ProtocolError::GY_PROTOCOL_INCOMPLETE){
                spdlog::warn("{} {} {} decode protocol error(fd: {}): {}",__TIME__,__FILE__,__LINE__,this->m_fd,Error::get_err_str(this->m_error));
                return -1;
            }
            this->m_rbuffer.erase(this->m_rbuffer.begin(), this->m_rbuffer.begin() + len);
            spdlog::info("{} {} {} read buffer clear(fd: {}) len: {}",__TIME__,__FILE__,__LINE__,this->m_fd,len);
            return 0;
        }

        int get_fixed_package()
        {
            if(this->m_rbuffer.length() < this->m_req->proto_fixed_len()) {
                spdlog::warn("{} {} {} protocol data incomplete (fd: {}) require len: {} , real len: {}",__TIME__,__FILE__,__LINE__,this->m_fd,this->m_req->proto_fixed_len(),this->m_rbuffer.length());
                return -1;
            }
            std::string res = this->m_rbuffer.substr(0 , this->m_req->proto_fixed_len());
            int len = this->m_req->decode(res, this->m_error);
            spdlog::info("{} {} {} decode protocol (fd: {}) len: {}",__TIME__,__FILE__,__LINE__,this->m_fd,len);
            if (this->m_error == Error::ProtocolError::GY_PROTOCOL_INCOMPLETE){
                spdlog::warn("{} {} {} decode protocol error(fd: {}): {}",__TIME__,__FILE__,__LINE__,this->m_fd,Error::get_err_str(this->m_error));
                return -1;
            }
            this->m_rbuffer.erase(this->m_rbuffer.begin(), this->m_rbuffer.begin() + m_req->proto_fixed_len());
            spdlog::info("{} {} {} read buffer clear(fd: {}) len: {}",__TIME__,__FILE__,__LINE__,this->m_fd, m_req->proto_fixed_len());
            return 0;
        }

        int get_fixed_head_package()
        {
            if(this->m_rbuffer.length() < this->m_req->proto_fixed_len()) {
                spdlog::warn("{} {} {} protocol data incomplete (fd: {}) require len: {} , real len: {}",__TIME__,__FILE__,__LINE__,this->m_fd,this->m_req->proto_fixed_len(),this->m_rbuffer.length());
                return -1;
            }
            std::string head = this->m_rbuffer.substr(0, this->m_req->proto_fixed_len());
            int len = m_req->decode(head,this->m_error);
            spdlog::info("{} {} {} decode protocol (fd: {}) len: {}",__TIME__,__FILE__,__LINE__,this->m_fd,len);
            if (this->m_error == Error::ProtocolError::GY_PROTOCOL_INCOMPLETE)
            {
                spdlog::warn("{} {} {} decode protocol error(fd: {}): {}",__TIME__,__FILE__,__LINE__,this->m_fd,Error::get_err_str(this->m_error));
                return -1;
            }
            int total_len = this->m_req->proto_fixed_len() + this->m_req->proto_extra_len();
            if(this->m_rbuffer.length() < total_len) {
                this->m_error = Error::ProtocolError::GY_PROTOCOL_INCOMPLETE;
                spdlog::warn("{} {} {} decode protocol error(fd: {}): {}",__TIME__,__FILE__,__LINE__,this->m_fd,Error::get_err_str(this->m_error));
                return -1;
            }
            std::string body = this->m_rbuffer.substr(this->m_req->proto_fixed_len(),this->m_req->proto_extra_len());
            this->m_req->set_extra_msg(std::move(body));
            this->m_rbuffer.erase(this->m_rbuffer.begin(), this->m_rbuffer.begin()+total_len);
            spdlog::info("{} {} {} read buffer clear(fd: {}) len: {}",__TIME__,__FILE__,__LINE__,this->m_fd, total_len);
            return 0;
        }


    protected:
        char *m_temp = nullptr;
        std::weak_ptr<Scheduler_Base> m_scheduler;
        int m_fd;
        uint32_t m_read_len;
        std::string m_rbuffer;
        std::string m_wbuffer;
        std::shared_ptr<REQ> m_req;
        std::shared_ptr<RESP> m_resp;
        Task<> m_co_task;
        Callback m_func;
        int m_error;
        //timer
        int m_conn_timeout;
        Timer::ptr m_timer;
    };

    template <Tcp_Request REQ, Tcp_Response RESP>
    class Tcp_Main_Task : public Task_Base
    {
    public:
        using ptr = std::shared_ptr<Tcp_Main_Task>;
        Tcp_Main_Task(int fd, std::vector<std::weak_ptr<Scheduler_Base>> schedulers,
                        std::function<Task<>(Task_Base::wptr)> &&func, int read_len,int conn_timeout)
        {
            this->m_fd = fd;
            this->m_status = Task_Status::GY_TASK_READ;
            this->m_schedulers = schedulers;
            this->m_func = func;
            this->m_read_len = read_len;
            this->m_conn_timeout = conn_timeout;
        }

        int exec() override
        {
            int connfd = IOFuntion::TcpFunction::Accept(this->m_fd);
            if (connfd <= 0)
            {
                spdlog::error("{} {} {} accept fail:{}",__TIME__,__FILE__,__LINE__,strerror(errno));
                return -1;
            }else{
                spdlog::info("{} {} {} accept success(fd: {})",__TIME__,__FILE__,__LINE__,connfd);
            }

            if (this->m_keepalive_conf.m_keepalive)
            {
                int ret = IOFuntion::TcpFunction::SockKeepalive(connfd, this->m_keepalive_conf.m_idle, this->m_keepalive_conf.m_interval, this->m_keepalive_conf.m_retry);
                if (ret == -1)
                {
                    close(connfd);
                    this->m_error = Error::NetError::GY_SETSOCKOPT_ERROR;
                    spdlog::error("{} {} {} socket set keepalive fail(fd: {} ):{} , close connection",__TIME__,__FILE__,__LINE__,connfd,strerror(errno));
                    return -1;
                }else spdlog::info("{} {} {} socket set keepalive success (fd: {} )",__TIME__,__FILE__,__LINE__,connfd);
            }
            if( IOFuntion::TcpFunction::IO_Set_No_Block(connfd) == -1 ){
                this->m_error = Error::NetError::GY_SET_NOBLOCK_ERROR;
                spdlog::error("{} {} {} SetNoBlock fail(fd: {} ):{} , close connection",__TIME__,__FILE__,__LINE__,connfd,strerror(errno));
                close(connfd);
                return -1;
            }else {
                spdlog::info("{} {} {} SetNoBlock success (fd: {} )",__TIME__,__FILE__,__LINE__,connfd);
            }
            int indx;
            if (m_schedulers.size() == 1)
            {
                indx = 0;
            }else{
                indx = connfd % (m_schedulers.size() - 1) + 1;
            }
            if (!this->m_schedulers[indx].expired())
            {
                auto task = create_rw_task(connfd, this->m_schedulers[indx]);
                this->m_schedulers[indx].lock()->add_task({connfd, task});
                spdlog::info("{} {} {} scheduler(indx: {}) add task success(fd: {})",__TIME__,__FILE__,__LINE__ , indx, connfd);
                if (this->m_schedulers[indx].lock()->add_event(connfd, GY_EVENT_READ | GY_EVENT_EPOLLET | GY_EVENT_ERROR) == -1)
                {
                    spdlog::error("{} {} {} scheduler add event fail(fd: {} ) {}, close connection",__TIME__,__FILE__,__LINE__,connfd,strerror(errno));
                    close(connfd);
                    return -1;
                }else spdlog::info("{} {} {} scheduler(indx: {}) add event success(fd: {})",__TIME__,__FILE__,__LINE__,indx,connfd);
            }
            return 0;
        }

        void enable_keepalive(uint16_t idle, uint16_t interval, uint16_t retry)
        {
            Tcp_Keepalive_Config keepalive{
                .m_keepalive = true,
                .m_idle = idle,
                .m_interval = interval,
                .m_retry = retry
            };
            this->m_keepalive_conf = keepalive;
        }

        virtual ~Tcp_Main_Task()
        {

        }

    protected:
        virtual Task_Base::ptr create_rw_task(int connfd,Scheduler_Base::wptr scheduler)
        {
            auto task = std::make_shared<Tcp_RW_Task<REQ,RESP>>(connfd, scheduler, this->m_read_len,this->m_conn_timeout);
            task->set_callback(std::forward<std::function<Task<>(Task_Base::wptr)>>(this->m_func));
            return task;
        }

    protected:
        int m_fd;
        std::vector<std::weak_ptr<Scheduler_Base>> m_schedulers;
        uint32_t m_read_len;
        std::function<Task<>(Task_Base::wptr)> m_func;
        int m_conn_timeout;
        // keepalive
        Tcp_Keepalive_Config m_keepalive_conf;
    };

    template <Tcp_Request REQ, Tcp_Response RESP>
    class Tcp_SSL_RW_Task : public Tcp_RW_Task<REQ,RESP>
    {
    public:
        using ptr = std::shared_ptr<Tcp_SSL_RW_Task>;
        Tcp_SSL_RW_Task(int fd, std::weak_ptr<Scheduler_Base> scheduler, uint32_t read_len,int conn_timeout, SSL *ssl)
            : Tcp_RW_Task<REQ,RESP>(fd, scheduler, read_len,conn_timeout), m_ssl(ssl)
        {
        }

        virtual ~Tcp_SSL_RW_Task()
        {
            if (this->m_ssl)
            {
                IOFuntion::TcpFunction::SSLDestory(this->m_ssl);
                this->m_ssl = nullptr;
            }
        }

    protected:
        int read_package() override
        {
            if(read_all_buffer() == -1) return -1;
            this->m_error = Error::NoError::GY_SUCCESS;
            switch (this->m_req->proto_type())
            {
                //buffer read empty
            case GY_ALL_RECIEVE_PROTOCOL_TYPE:
                return this->get_all_buffer();
                //fixed proto package
            case GY_PACKAGE_FIXED_PROTOCOL_TYPE:
                return this->get_fixed_package();
                //fixed head and head includes body's length
            case GY_HEAD_FIXED_PROTOCOL_TYPE:
                return this->get_fixed_head_package();
            }
            return -1;
        }

        int send_package() override
        {
            int len;
            if(this->m_wbuffer.empty()) return 0;
            do{
                len = IOFuntion::TcpFunction::SSLSend(this->m_ssl, this->m_wbuffer, this->m_wbuffer.length());
                if (len != -1 && len != 0) {
                    spdlog::info("{} {} {} SSL_Send (fd: {}) {} Bytes",__TIME__,__FILE__,__LINE__,this->m_fd,len);
                    this->m_wbuffer.erase(this->m_wbuffer.begin(), this->m_wbuffer.begin() + len);
                }
                if(this->m_wbuffer.empty()) break;
            }while(len != 0 && len != -1);
            if (len == -1)
            {
                if (errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN)
                {
                    spdlog::error("{} {} {} SSL_Send (fd: {}) occurs error: {}, close connection",__TIME__,__FILE__,__LINE__,this->m_fd,strerror(errno));
                    IOFuntion::TcpFunction::SSLDestory(this->m_ssl);
                    this->m_ssl = nullptr;
                    Task_Base::destory();
                    return -1;
                }
            }
            else if (len == 0)
            {
                spdlog::error("{} {} {} SSL_Send (fd: {}) occurs error: {}, close connection",__TIME__,__FILE__,__LINE__,this->m_fd,strerror(errno));
                IOFuntion::TcpFunction::SSLDestory(this->m_ssl);
                this->m_ssl = nullptr;
                Task_Base::destory();
                return -1;
            } 
            return 0;
        }

        int read_all_buffer()
        {
            int len;
            do
            {
                memset(this->m_temp, 0, this->m_read_len);
                len = IOFuntion::TcpFunction::SSLRecv(this->m_ssl, this->m_temp, this->m_read_len);
                if (len != -1 && len != 0){
                    spdlog::info("{} {} {} SSL_Recv (fd: {}) {} Bytes",__TIME__,__FILE__,__LINE__,this->m_fd,len);
                    this->m_rbuffer.append(this->m_temp, len);
                }
            } while (len != -1 && len != 0);
            if (len == -1)
            {
                if (errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN)
                {
                    spdlog::error("{} {} {} SSL_Recv (fd: {}) occurs error: {}, close connection",__TIME__,__FILE__,__LINE__,this->m_fd,strerror(errno));
                    Task_Base::destory();
                    IOFuntion::TcpFunction::SSLDestory(this->m_ssl);
                    this->m_ssl = nullptr;
                    return -1;
                }
            }
            else if (len == 0)
            {
                spdlog::error("{} {} {} SSL_Recv (fd: {}) The peer closes the connection, error: {}, close connection",__TIME__,__FILE__,__LINE__,this->m_fd,strerror(errno));
                Task_Base::destory();
                IOFuntion::TcpFunction::SSLDestory(this->m_ssl);
                this->m_ssl = nullptr;
                return -1;
            }
            return 0;
        }

    protected:
        SSL *m_ssl = nullptr;
    };

    // tcp's ssl task
    template <Tcp_Request REQ, Tcp_Response RESP>
    class Tcp_SSL_Main_Task : public Tcp_Main_Task<REQ,RESP>
    {
    public:
        using ptr = std::shared_ptr<Tcp_SSL_Main_Task>;
        Tcp_SSL_Main_Task(int fd, std::vector<std::weak_ptr<Scheduler_Base>> schedulers, std::function<Task<>(Task_Base::wptr)> &&func,
                            uint32_t read_len, int conn_timeout, uint32_t ssl_accept_max_retry, SSL_CTX *ctx)
            : Tcp_Main_Task<REQ,RESP>(fd, schedulers, std::forward<std::function<Task<>(Task_Base::wptr)>>(func), read_len,conn_timeout),
              m_ctx(ctx), m_ssl_accept_retry(ssl_accept_max_retry)
        {
        }

        int exec() override
        {
            int connfd = IOFuntion::TcpFunction::Accept(this->m_fd);
            if (connfd <= 0){
                spdlog::error("{} {} {} accept fail:{} ",__TIME__,__FILE__,__LINE__,strerror(errno));
                return -1;
            }
            if (this->m_keepalive_conf.m_keepalive)
            {
                if( IOFuntion::TcpFunction::SockKeepalive(connfd, this->m_keepalive_conf.m_idle, this->m_keepalive_conf.m_interval, this->m_keepalive_conf.m_retry) == -1){
                    close(connfd);
                    spdlog::error("{} {} {} socket set keepalive fail(fd: {} ):{} , close connection",__TIME__,__FILE__,__LINE__,connfd,strerror(errno));
                    return -1;
                }else spdlog::info("{} {} {} socket set keepalive success (fd: {} )",__TIME__,__FILE__,__LINE__,connfd);
            }
            SSL *ssl = IOFuntion::TcpFunction::SSLCreateObj(this->m_ctx, connfd);
            if (ssl == nullptr)
            {
                this->m_error = Error::NetError::GY_SSL_OBJ_INIT_ERROR;
                spdlog::error("{} {} {} SSLCreateObj fail(fd: {} ):{} , close connection",__TIME__,__FILE__,__LINE__,connfd,strerror(errno));
                close(connfd);
                return -1;
            }else{
                spdlog::info("{} {} {} SSLCreateObj success (fd: {} )",__TIME__,__FILE__,__LINE__,connfd);
            }
            if( IOFuntion::TcpFunction::IO_Set_No_Block(connfd) == -1 ){
                this->m_error = Error::NetError::GY_SET_NOBLOCK_ERROR;
                spdlog::error("{} {} {} SetNoBlock fail(fd: {} ):{} , close connection",__TIME__,__FILE__,__LINE__,connfd,strerror(errno));
                close(connfd);
                return -1;
            }else {
                spdlog::info("{} {} {} SetNoBlock success (fd: {} )",__TIME__,__FILE__,__LINE__,connfd);
            }
            int ret = IOFuntion::TcpFunction::SSLAccept(ssl);
            uint32_t retry = 0;
            while (ret <= 0 && retry++ <= this->m_ssl_accept_retry)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_SSL_SLEEP_MISC_PER_RETRY));
                ret = IOFuntion::TcpFunction::SSLAccept(ssl);
                if (ret <= 0)
                {
                    int ssl_err = SSL_get_error(ssl, ret);
                    if (ssl_err == SSL_ERROR_WANT_READ || ssl_err == SSL_ERROR_WANT_WRITE || ssl_err == SSL_ERROR_WANT_ACCEPT)
                    {
                        continue;
                    }
                    else
                    {
                        this->m_error = Error::NetError::GY_SSL_ACCEPT_ERROR;
                        char msg[256];
                        ERR_error_string(ssl_err,msg);
                        spdlog::error("{} {} {} SSL_Accept fail(fd: {} ):{} , retry: {} , close connection",__TIME__,__FILE__,__LINE__,connfd,msg,retry);
                        IOFuntion::TcpFunction::SSLDestory(ssl);
                        ssl = nullptr;
                        close(connfd);
                        return -1;
                    }
                }
            }
            spdlog::info("{} {} {} SSL_Accept success (fd: {} ) , retry: {}",__TIME__,__FILE__,__LINE__,connfd,retry);
            int indx;
            if (this->m_schedulers.size() == 1)
            {
                indx = 0;
            }else{
                indx = connfd % (this->m_schedulers.size() - 1) + 1;
            }

            if (!this->m_schedulers[indx].expired())
            {
                auto task = create_rw_task(connfd, ssl, this->m_schedulers[indx]);
                this->m_schedulers[indx].lock()->add_task({connfd, task});
                if (this->m_schedulers[indx].lock()->add_event(connfd, GY_EVENT_READ | GY_EVENT_EPOLLET | GY_EVENT_ERROR) == -1)
                {
                    spdlog::error("{} {} {} scheduler add event fail(fd: {} ) {}, close connection",__TIME__,__FILE__,__LINE__,connfd,strerror(errno));
                    IOFuntion::TcpFunction::SSLDestory(ssl);
                    ssl = nullptr;
                    close(connfd);
                    return -1;
                }else spdlog::info("{} {} {} scheduler add success(fd: {} )",__TIME__,__FILE__,__LINE__,connfd);
            }
            return 0;
        }

        virtual ~Tcp_SSL_Main_Task()
        {
            if (this->m_ctx)
                this->m_ctx = nullptr;
        }

    protected:
        virtual Task_Base::ptr create_rw_task(int connfd, SSL *ssl,Scheduler_Base::wptr scheduler)
        {
            auto task = std::make_shared<Tcp_SSL_RW_Task<REQ,RESP>>(connfd, scheduler, this->m_read_len,this->m_conn_timeout, ssl);
            task->set_callback(std::forward<std::function<Task<>(Task_Base::wptr)>>(this->m_func));
            return task;
        }

    protected:
        SSL_CTX *m_ctx;
        uint32_t m_ssl_accept_retry;
    };

    

    // time task
    class Time_Task : public Task_Base
    {
    public:
        using ptr = std::shared_ptr<Time_Task>;
        Time_Task(std::weak_ptr<Timer_Manager> manager);

        int exec() override;

        ~Time_Task();

    protected:
        std::weak_ptr<Timer_Manager> m_manager;
    };

    // thread_task
    class Thread_Task : public Task_Base
    {
    public:
        using ptr = std::shared_ptr<Thread_Task>;
        Thread_Task(std::function<void()> &&func);

        int exec() override;

        ~Thread_Task();

    private:
        std::function<void()> m_func;
    };
}

#endif