#ifndef GALAY_CO_TASK_H
#define GALAY_CO_TASK_H

#include "task.h"
#include "../protocol/tcp.h"
#include "../protocol/http.h"


namespace galay
{

    template<typename RESULT>
    class Co_Task_Base : public Task_Base
    {
    public:
        using ptr = std::shared_ptr<Co_Task_Base>;


        virtual void set_co_handle(std::coroutine_handle<> handle)
        {
            this->m_handle = handle;
        }

        virtual int exec() = 0 ;

        RESULT& result()
        {
            return this->m_result;
        }

        virtual ~Co_Task_Base()
        {
            if(m_handle){
                m_handle = nullptr;
            }
        }

    protected:
        RESULT m_result;
        std::coroutine_handle<> m_handle;
    };

    template<typename RESULT = int>
    class Co_Tcp_Client_Connect_Task: public Co_Task_Base<RESULT>
    {
    public:
        using ptr = std::shared_ptr<Co_Tcp_Client_Connect_Task<RESULT>>;

        Co_Tcp_Client_Connect_Task(int fd , int *error)
        {
            this->m_fd = fd;
            this->m_error = error;
        }

        int exec() override
        {
            int status = 0;
            socklen_t slen = sizeof(status);
            if(getsockopt(this->m_fd,SOL_SOCKET,SO_ERROR,(void*)&status,&slen) < 0){
                *(this->m_error) = error::GY_CONNECT_ERROR;
                this->m_result = -1;
            }
            if(status != 0){
                *(this->m_error) = error::GY_CONNECT_ERROR;
                this->m_result = -1;
            }else{
                *(this->m_error) = error::GY_SUCCESS;
                this->m_result = 0;
            }
            if(!this->m_handle.done()) this->m_handle.resume();
            return 0;
        }

    protected:
        int m_fd;
        int *m_error;
    };


    template<typename RESULT = int>
    class Co_Tcp_Client_Send_Task:public Co_Task_Base<RESULT>
    {
    public:
        using ptr = std::shared_ptr<Co_Tcp_Client_Send_Task>;
        Co_Tcp_Client_Send_Task(int fd ,const std::string &buffer , uint32_t len , int *error)
        {
            this->m_fd = fd;
            this->m_buffer = buffer;
            this->m_len = len;
            this->m_error = error;
        }

        int exec() override
        {
            int ret = iofunction::Tcp_Function::Send(this->m_fd,this->m_buffer,this->m_len);
            if(ret == -1){
                if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
                {
                    return -1;
                }else{
                    *(this->m_error) = error::GY_SEND_ERROR;
                    this->m_result = -1;
                }
            }else if(ret == 0){
                *(this->m_error) = error::GY_SEND_ERROR;
                this->m_result = -1;
            }else{
                this->m_result = ret;
                *(this->m_error) = error::GY_SUCCESS;
            }
            if(!this->m_handle.done()) this->m_handle.resume();
            return 0;
        }

    protected:
        int m_fd;
        std::string m_buffer;
        uint32_t m_len;
        int * m_error;
    };

    template<typename RESULT = int>
    class Co_Tcp_Client_Recv_Task:public Co_Task_Base<RESULT>
    {
    public:
        using ptr = std::shared_ptr<Co_Tcp_Client_Recv_Task>;
        Co_Tcp_Client_Recv_Task(int fd , char* buffer,int len , int *error)
        {
            this->m_fd = fd;
            this->m_buffer = buffer;
            this->m_len = len;
            this->m_error = error;
        }
 
        int exec() override
        {
            int ret = iofunction::Tcp_Function::Recv(this->m_fd,this->m_buffer,this->m_len);
            if(ret == -1){
                if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
                {
                    return -1;
                }else{
                    *(this->m_error) = error::GY_RECV_ERROR;
                    this->m_result = -1;
                }
            }else if(ret == 0){
                *(this->m_error) = error::GY_RECV_ERROR;
                this->m_result = -1;
            }else{
                *(this->m_error) = error::GY_SUCCESS;
                this->m_result = ret;
            }
            if(!this->m_handle.done()) this->m_handle.resume();
            return 0;
        }

    protected:
        int m_fd;
        char* m_buffer = nullptr;
        int m_len;
        int * m_error;
    };

    template<typename RESULT = int>
    class Http_Request_Task: public Co_Task_Base<RESULT>
    {
    public:
        using ptr = std::shared_ptr<Http_Request_Task>;
        Http_Request_Task(int fd , Scheduler_Base::wptr scheduler , Http_Request::ptr request , Http_Response::ptr response , int *error)
        {
            this->m_fd = fd;
            this->m_request = request;
            this->m_respnse = response;
            this->m_status = Task_Status::GY_TASK_WRITE;
            this->m_scheduler = scheduler;
            this->m_tempbuffer = new char[DEFAULT_RECV_LENGTH];
            this->m_error = error;
        }

        int exec() override
        {
            switch (this->m_status)
            {
            case Task_Status::GY_TASK_WRITE :
            {
                std::string request = m_request->encode();
                int ret = iofunction::Tcp_Function::Send(this->m_fd, request, request.length());
                if (ret == -1)
                {
                    if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
                    {
                        return -1;
                    }
                    else
                    {
                        *(this->m_error) = error::GY_SEND_ERROR;
                        this->m_result = -1;
                    }
                }
                else if (ret == 0)
                {
                    *(this->m_error) = error::GY_SEND_ERROR;
                    this->m_result = -1;
                }
                else if( ret == request.length())
                {
                    this->m_result = ret;
                    this->m_status = Task_Status::GY_TASK_READ;
                    *(this->m_error) = error::GY_SUCCESS;
                    if(!m_scheduler.expired()){
                        m_scheduler.lock()->mod_event(this->m_fd , EPOLLIN);
                    }
                    return -1;
                }else{
                    return -1;
                }
                break;
            }
            case Task_Status::GY_TASK_READ:
            {
                int ret;
                do{
                    memset(m_tempbuffer,0,DEFAULT_RECV_LENGTH);
                    ret = iofunction::Tcp_Function::Recv(this->m_fd, m_tempbuffer, DEFAULT_RECV_LENGTH);
                    if(ret != -1 && ret != 0) this->m_buffer.append(m_tempbuffer,ret);
                }while(ret != -1 && ret != 0);
                if(ret == -1){
                    if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
                    {
                        return -1;
                    }else
                    {
                        *(this->m_error) = error::GY_RECV_ERROR;
                        this->m_result = -1;
                        if (!this->m_handle.done()) this->m_handle.resume();
                        return -1;
                    }
                }
                else if(ret == 0)
                {
                    *(this->m_error) = error::GY_RECV_ERROR;
                    this->m_result = -1;
                }
                int state = 0;
                this->m_respnse->decode(this->m_buffer, state);
                if (state == error::protocol_error::GY_PROTOCOL_INCOMPLETE)
                {
                    return -1;
                }
                else
                {
                    *(this->m_error) = state;
                }
                *(this->m_error) = error::GY_SUCCESS;
                this->m_result = 0;
                break;
            }
            default:
                break;
            }
            if (!this->m_handle.done()) this->m_handle.resume();
            return 0;
        }

        ~Http_Request_Task()
        {
            if(m_tempbuffer){
                delete[] m_tempbuffer;
                m_tempbuffer = nullptr;
            }
        }


    protected:
        int m_fd;
        char* m_tempbuffer;
        std::string m_buffer;
        Http_Request::ptr m_request;
        Http_Response::ptr m_respnse;
        Scheduler_Base::wptr m_scheduler;
        int* m_error;
    };

    template<typename RESULT = int>
    class Co_Tcp_Client_SSL_Connect_Task: public Co_Task_Base<RESULT>
    {
    public:
        using ptr = std::shared_ptr<Co_Tcp_Client_SSL_Connect_Task<RESULT>>;

        Co_Tcp_Client_SSL_Connect_Task(int fd , SSL* ssl , int *error , int init_status)
        {
            this->m_fd = fd;
            this->m_error = error;
            this->m_status = init_status;
            this->m_ssl = ssl;
        }

        int exec() override
        {

            switch (this->m_status)
            {
            case Task_Status::GY_TASK_CONNECT:
            {
                int status = 0;
                socklen_t slen = sizeof(status);
                if (getsockopt(this->m_fd, SOL_SOCKET, SO_ERROR, (void *)&status, &slen) < 0)
                {
                    *(this->m_error) = error::GY_CONNECT_ERROR;
                    this->m_result = -1;
                }
                if (status != 0)
                {
                    *(this->m_error) = error::GY_CONNECT_ERROR;
                    this->m_result = -1;
                }
                else
                {
                    this->m_status = Task_Status::GY_TASK_SSL_CONNECT;
                    return -1;
                }
                break;
            }
            case Task_Status::GY_TASK_SSL_CONNECT:
            {
                int ret = iofunction::Tcp_Function::SSL_Connect(this->m_ssl);
                if(ret <= 0)
                {
                    int status = SSL_get_error(this->m_ssl,ret);
                    if(status == SSL_ERROR_WANT_READ || status == SSL_ERROR_WANT_WRITE || status == SSL_ERROR_WANT_CONNECT)
                    {
                        return -1;
                    }else{
                        *(this->m_error) = error::GY_SSL_CONNECT_ERROR;
                        this->m_result = -1;
                    }
                }else{
                    *(this->m_error) = error::GY_SUCCESS;
                    this->m_result = 0;
                }
                break;
            }
            }

            if(!this->m_handle.done()) this->m_handle.resume();
            return 0;
        }
    protected:
        int m_fd;
        int *m_error;
        SSL* m_ssl;
    };
    
    template<typename RESULT = int>
    class Co_Tcp_Client_SSL_Send_Task:public Co_Task_Base<RESULT>
    {
    public:
        using ptr = std::shared_ptr<Co_Tcp_Client_SSL_Send_Task>;
        Co_Tcp_Client_SSL_Send_Task(SSL * ssl,const std::string &buffer , uint32_t len , int *error)
        {
            this->m_ssl = ssl;
            this->m_buffer = buffer;
            this->m_len = len;
            this->m_error = error;
        }

        int exec() override
        {
            int ret = iofunction::Tcp_Function::SSL_Send(this->m_ssl,this->m_buffer,this->m_len);
            if(ret == -1){
                if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
                {
                    return -1;
                }else{
                    *(this->m_error) = error::GY_SSL_SEND_ERROR;
                    this->m_result = -1;
                }
            }else if(ret == 0){
                *(this->m_error) = error::GY_SSL_SEND_ERROR;
                this->m_result = -1;
            }else{
                this->m_result = ret;
                *(this->m_error) = error::GY_SUCCESS;
            }
            if(!this->m_handle.done()) this->m_handle.resume();
            return 0;
        }

    protected:
        SSL* m_ssl;
        std::string m_buffer;
        uint32_t m_len;
        int * m_error;
    };

    template<typename RESULT = int>
    class Co_Tcp_Client_SSL_Recv_Task:public Co_Task_Base<RESULT>
    {
    public:
        using ptr = std::shared_ptr<Co_Tcp_Client_SSL_Recv_Task>;
        Co_Tcp_Client_SSL_Recv_Task(SSL* ssl , char* buffer,int len , int *error)
        {
            this->m_ssl = ssl;
            this->m_buffer = buffer;
            this->m_len = len;
            this->m_error = error;
        }
 
        int exec() override
        {
            int ret = iofunction::Tcp_Function::SSL_Recv(this->m_ssl,this->m_buffer,this->m_len);
            if(ret == -1){
                if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
                {
                    return -1;
                }else{
                    *(this->m_error) = error::GY_SSL_RECV_ERROR;
                    this->m_result = -1;
                }
            }else if(ret == 0){
                *(this->m_error) = error::GY_SSL_RECV_ERROR;
                this->m_result = -1;
            }else{
                *(this->m_error) = error::GY_SUCCESS;
                this->m_result = ret;
            }
            if(!this->m_handle.done()) this->m_handle.resume();
            return 0;
        }

    protected:
        SSL* m_ssl;
        char* m_buffer = nullptr;
        int m_len;
        int * m_error;
    };


    template<typename RESULT = int>
    class Https_Request_Task: public Co_Task_Base<RESULT>
    {
    public:
        using ptr = std::shared_ptr<Https_Request_Task>;
        Https_Request_Task(SSL* ssl , int fd , Scheduler_Base::wptr scheduler , Http_Request::ptr request , Http_Response::ptr response , int *error)
        {
            this->m_fd = fd;
            this->m_ssl = ssl;
            this->m_request = request;
            this->m_respnse = response;
            this->m_status = Task_Status::GY_TASK_WRITE;
            this->m_scheduler = scheduler;
            this->m_tempbuffer = new char[DEFAULT_RECV_LENGTH];
            this->m_error = error;
        }

        int exec() override
        {
            switch (this->m_status)
            {
            case Task_Status::GY_TASK_WRITE :
            {
                std::string request = m_request->encode();
                int ret = iofunction::Tcp_Function::SSL_Send(this->m_ssl, request, request.length());
                if (ret == -1)
                {
                    if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
                    {
                        return -1;
                    }
                    else
                    {
                        *(this->m_error) = error::GY_SEND_ERROR;
                        this->m_result = -1;
                    }
                }
                else if (ret == 0)
                {
                    *(this->m_error) = error::GY_SEND_ERROR;
                    this->m_result = -1;
                }
                else if( ret == request.length())
                {
                    this->m_result = ret;
                    this->m_status = Task_Status::GY_TASK_READ;
                    *(this->m_error) = error::GY_SUCCESS;
                    if(!m_scheduler.expired()) m_scheduler.lock()->mod_event(this->m_fd , EPOLLIN);
                    return -1;
                }else{
                    return -1;
                }
                break;
            }
            case Task_Status::GY_TASK_READ:
            {
                int ret;
                do{
                    memset(m_tempbuffer,0,DEFAULT_RECV_LENGTH);
                    ret = iofunction::Tcp_Function::SSL_Recv(this->m_ssl, m_tempbuffer, DEFAULT_RECV_LENGTH);
                    if(ret != -1 && ret != 0) this->m_buffer.append(m_tempbuffer,ret);
                }while(ret != -1 && ret != 0);
                if(ret == -1){
                    if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
                    {
                        return -1;
                    }else
                    {
                        *(this->m_error) = error::GY_RECV_ERROR;
                        this->m_result = -1;
                        if (!this->m_handle.done()) this->m_handle.resume();
                        return -1;
                    }
                }
                else if(ret == 0)
                {
                    *(this->m_error) = error::GY_RECV_ERROR;
                    this->m_result = -1;
                }
                int state = 0;
                this->m_respnse->decode(this->m_buffer, state);
                if (state == error::protocol_error::GY_PROTOCOL_INCOMPLETE)
                {
                    return -1;
                }
                else
                {
                    *(this->m_error) = state;
                }
                *(this->m_error) = error::GY_SUCCESS;
                this->m_result = 0;
                break;
            }
            default:
                break;
            }
            if (!this->m_handle.done()) this->m_handle.resume();
            return 0;
        }

        ~Https_Request_Task()
        {
            if(m_tempbuffer){
                delete[] m_tempbuffer;
                m_tempbuffer = nullptr;
            }
        }


    protected:
        int m_fd;
        SSL* m_ssl;
        char* m_tempbuffer;
        std::string m_buffer;
        Http_Request::ptr m_request;
        Http_Response::ptr m_respnse;
        Scheduler_Base::wptr m_scheduler;
        int* m_error;
    };


}

#endif