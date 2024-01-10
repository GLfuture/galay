#ifndef GALAY_CO_TASK_H
#define GALAY_CO_TASK_H

#include "task.h"

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

        bool is_need_to_destroy() override
        {
            return this->m_is_finish;
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

        Co_Tcp_Client_Connect_Task(int fd)
        {
            this->m_fd = fd;
        }

        int exec() override
        {
            int status = 0;
            socklen_t slen = sizeof(status);
            if(getsockopt(this->m_fd,SOL_SOCKET,SO_ERROR,(void*)&status,&slen) < 0){
                this->m_result = -1;
            }
            if(status != 0){
                this->m_error = error::GY_CONNECT_ERROR;
                this->m_result = -1;
            }else{
                this->m_error = error::GY_CONNECT_ERROR;
                this->m_result = 0;
            }
            if(!this->m_handle.done()) this->m_handle.resume();
            return 0;
        }

        bool is_need_to_destroy() override
        {
            return Co_Task_Base<RESULT>::is_need_to_destroy();
        }
    protected:
        int m_fd;
    };


    template<typename RESULT = int>
    class Co_Tcp_Client_Send_Task:public Co_Task_Base<RESULT>
    {
    public:
        using ptr = std::shared_ptr<Co_Tcp_Client_Send_Task>;
        Co_Tcp_Client_Send_Task(int fd ,const std::string &buffer , uint32_t len)
        {
            this->m_fd = fd;
            this->m_buffer = buffer;
            this->m_len = len;
        }

        int exec() override
        {
            int ret = iofunction::Tcp_Function::Send(this->m_fd,this->m_buffer,this->m_len);
            if(ret == -1){
                if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
                {
                    return -1;
                }else{
                    this->m_error = error::GY_SEND_ERROR;
                    this->m_result = -1;
                }
            }else if(ret == 0){
                this->m_error = error::GY_SEND_ERROR;
                this->m_result = -1;
            }else{
                this->m_result = ret;
            }
            if(!this->m_handle.done()) this->m_handle.resume();
            return 0;
        }

        bool is_need_to_destroy() override
        {
            return Co_Task_Base<RESULT>::is_need_to_destroy();
        }
    protected:
        int m_fd;
        std::string m_buffer;
        uint32_t m_len;
    };

    template<typename RESULT = int>
    class Co_Tcp_Client_Recv_Task:public Co_Task_Base<RESULT>
    {
    public:
        using ptr = std::shared_ptr<Co_Tcp_Client_Recv_Task>;
        Co_Tcp_Client_Recv_Task(int fd,char* buffer,int len)
        {
            this->m_fd = fd;
            this->m_buffer = buffer;
            this->m_len = len;
        }
 
        int exec() override
        {
            int ret = iofunction::Tcp_Function::Recv(this->m_fd,this->m_buffer,this->m_len);
            if(ret == -1){
                if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
                {
                    return -1;
                }else{
                    this->m_error = error::GY_RECV_ERROR;
                    this->m_result = -1;
                }
            }else if(ret == 0){
                this->m_error = error::GY_RECV_ERROR;
                this->m_result = -1;
            }else{
                this->m_result = ret;
            }
            if(!this->m_handle.done()) this->m_handle.resume();
            return 0;
        }

        bool is_need_to_destroy() override
        {
            return this->m_is_finish;
        }
    protected:
        int m_fd;
        char* m_buffer = nullptr;
        int m_len;
    };

    template<typename RESULT = int>
    class Http_Request_Task: public Co_Task_Base<RESULT>
    {
    public:
        using ptr = std::shared_ptr<Http_Request_Task>;
        Http_Request_Task(int fd , Engine::ptr engine , Http_Request::ptr request , Http_Response::ptr response)
        {
            this->m_fd = fd;
            this->m_request = request;
            this->m_respnse = response;
            this->m_status = Task_Status::GY_TASK_WRITE;
            this->m_engine = engine;
            this->m_tempbuffer = new char[DEFAULT_RECV_LENGTH];
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
                        this->m_error = error::GY_SEND_ERROR;
                        this->m_result = -1;
                    }
                }
                else if (ret == 0)
                {
                    this->m_error = error::GY_SEND_ERROR;
                    this->m_result = -1;
                }
                else
                {
                    this->m_result = ret;
                    this->m_status = Task_Status::GY_TASK_READ;
                    m_engine->mod_event(this->m_fd , EPOLLIN);
                    return -1;
                }
                break;
            }
            case Task_Status::GY_TASK_READ:
            {
                memset(m_tempbuffer,0,DEFAULT_RECV_LENGTH);
                int ret = iofunction::Tcp_Function::Recv(this->m_fd, m_tempbuffer, DEFAULT_RECV_LENGTH);
                if (ret == -1)
                {
                    if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
                    {
                        return -1;
                    }
                    else
                    {
                        this->m_error = error::GY_RECV_ERROR;
                        this->m_result = -1;
                    }
                }
                else if (ret == 0)
                {
                    this->m_error = error::GY_RECV_ERROR;
                    this->m_result = -1;
                }
                else
                {
                    int state = 0;
                    this->m_buffer.append(m_tempbuffer,ret);
                    this->m_respnse->decode(this->m_buffer,state);
                    if(state == error::protocol_error::GY_PROTOCOL_INCOMPLETE){
                        return -1;
                    }else{
                        this->m_error = state;
                    }
                    this->m_result = ret;
                    
                }
                break;
            }
            default:
                break;
            }
            if (!this->m_handle.done()) this->m_handle.resume();
            return 0;
        }


        bool is_need_to_destroy() override
        {
            return this->m_is_finish;
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
        Engine::ptr m_engine;
    };

}

#endif