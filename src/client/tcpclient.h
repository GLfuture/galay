#ifndef GALAY_TCP_CLIENT_H
#define GALAY_TCP_CLIENT_H

#include "client.h"
#include "../kernel/awaiter.h"
#include <errno.h>
#include <cstring>

namespace galay
{
    template<Request REQ,Response RESP>
    class Tcp_Client: public Client<REQ,RESP>
    {
    public:
        using ptr = std::shared_ptr<Tcp_Client>;
        //创建时自动sock
        Tcp_Client(IO_Scheduler<REQ,RESP>::ptr scheduler)
            :Client<REQ,RESP>(scheduler)
        {
            this->m_fd = iofunction::Tcp_Function::Sock();
            iofunction::Simple_Fuction::IO_Set_No_Block(this->m_fd);
            this->m_scheduler->m_engine->add_event(this->m_fd , EPOLLET | EPOLLIN);
        }

        //0 is success  -1 is failed
        Net_Awaiter<REQ,RESP,int> connect(std::string ip , uint32_t port)
        {
            typename Co_Tcp_Client_Connect_Task<REQ,RESP,int>::ptr task = std::make_shared<Co_Tcp_Client_Connect_Task<REQ,RESP,int>>(this->m_fd);
            
            int ret = iofunction::Tcp_Function::Conncet(this->m_fd,ip,port);
            if(ret == 0){ 
                return Net_Awaiter<REQ,RESP,int>{nullptr , ret};
            }else if(ret == -1){
                if(errno != EINPROGRESS && errno != EINTR)
                {
                    return Net_Awaiter<REQ,RESP,int>{ nullptr , ret};
                }
            }
            Client<REQ,RESP>::add_task(task);
            this->m_scheduler->m_engine->mod_event(this->m_fd,EPOLLOUT);
            return Net_Awaiter<REQ,RESP,int>{task};
        }
        
        //-1 is failed >0 is successful
        //if >0 return the value that send's length
        Net_Awaiter<REQ,RESP,int> send(const std::string &buffer,uint32_t len)
        {
            int ret = iofunction::Tcp_Function::Send(this->m_fd,buffer,len);
            if(ret == 0){
                return Net_Awaiter<REQ,RESP,int>{nullptr,-1};
            }
            else if( ret == -1)
            {
                if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
                {
                    typename Co_Tcp_Client_Send_Task<REQ,RESP,int>::ptr task = std::make_shared<Co_Tcp_Client_Send_Task<REQ,RESP,int>>(this->m_fd,buffer,len);
                    Client<REQ,RESP>::add_task(task);
                    this->m_scheduler->m_engine->mod_event(this->m_fd,EPOLLOUT);
                    return Net_Awaiter<REQ,RESP,int>{task};
                }else{
                    return Net_Awaiter<REQ,RESP,int>{nullptr,ret};
                }
            }
            return Net_Awaiter<REQ,RESP,int>{nullptr,ret};
        }
        
        //-1 is failed >0 is success 
        //if >0 return the value that recv's length
        Net_Awaiter<REQ,RESP,int> recv(char* buffer,int len)
        {
            int ret = iofunction::Tcp_Function::Recv(this->m_fd , buffer, len);
            if(ret == 0){
                return Net_Awaiter<REQ,RESP,int>{nullptr,-1};
            }
            else if(ret == -1)
            {
                if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN){
                    typename Co_Tcp_Client_Recv_Task<REQ,RESP,int>::ptr task = std::make_shared<Co_Tcp_Client_Recv_Task<REQ,RESP,int>>(this->m_fd,buffer,len);
                    Client<REQ,RESP>::add_task(task);
                    this->m_scheduler->m_engine->mod_event(this->m_fd,EPOLLIN);
                    return Net_Awaiter<REQ,RESP,int>{task};
                }else{
                    return Net_Awaiter<REQ,RESP,int>{nullptr,-1};
                }
            }
            return Net_Awaiter<REQ,RESP,int>{nullptr,ret};
        }

        void disconnect()
        {
            close(this->m_fd);
            this->m_scheduler->m_engine->del_event(this->m_fd,EPOLLOUT|EPOLLIN);
            this->m_scheduler->m_tasks->erase(this->m_fd);
        }
    };

    template<Request REQ,Response RESP>
    class Tcp_SSL_Client: public Tcp_Client<REQ,RESP>
    {
    public:
        Tcp_SSL_Client(IO_Scheduler<REQ,RESP>::ptr scheduler, long ssl_min_version , long ssl_max_version, uint32_t ssl_connect_max_retry )
            :Tcp_Client<REQ,RESP>(scheduler)
        {
            this->m_ctx = iofunction::Tcp_Function::SSL_Init_Client(ssl_min_version,ssl_max_version);
            if(this->m_ctx == nullptr){
                this->m_error = error::server_error::GY_SSL_CTX_INIT_ERROR;
                ERR_print_errors_fp(stderr);
                exit(-1);
            }
            this->m_ssl = iofunction::Tcp_Function::SSL_Create_Obj(this->m_ctx,this->m_fd);
            if(this->m_ssl == nullptr){
                close(this->m_fd);
                exit(-1);
            }
        }




    protected:
        SSL* m_ssl;
        SSL_CTX* m_ctx;
        uint32_t m_ssl_connect_retry;
    };
}



#endif