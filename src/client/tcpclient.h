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
            this->m_scheduler->m_engine->add_event(this->m_fd,EPOLLOUT);
            return Net_Awaiter<REQ,RESP,int>{task};
        }
        
        //-1 is failed >0 is successful
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
                    this->m_scheduler->m_engine->mod_event(this->m_fd,EPOLLOUT);
                }else{
                    return Net_Awaiter<REQ,RESP,int>{nullptr,ret};
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
}



#endif