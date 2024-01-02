#ifndef GALAY_TCP_CLIENT_H
#define GALAY_TCP_CLIENT_H

#include "client.h"
#include "../kernel/awaiter.h"

namespace galay
{
    template<Request REQ,Response RESP>
    class Tcp_Client: public Client<REQ,RESP>
    {
    public:
        Tcp_Client(IO_Scheduler<REQ,RESP>::ptr scheduler)
            :Client<REQ,RESP>(scheduler)
        {

        }

        Net_Awaiter<REQ,RESP,int> connect(std::string ip , uint32_t port)
        {
            Net_Awaiter<REQ,RESP,int>::ptr awaiter = std::make_shared<Net_Awaiter<REQ,RESP,int>>();
            this->m_fd = iofunction::Tcp_Function::Sock();
            int ret = iofunction::Simple_Fuction::IO_Set_No_Block(this->m_fd);
            if(ret != 0){

            }
            iofunction::Tcp_Function::Conncet(ip,port);
        }

    };
}



#endif