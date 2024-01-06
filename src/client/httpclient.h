#ifndef GALAY_HTTP_CLIENT_H
#define GALAY_HTTP_CLIENT_H

#include "tcpclient.h"

namespace galay
{
    class Http_Client: public Tcp_Client
    {
    public:
        using ptr = std::shared_ptr<Http_Client>;
        Http_Client(IO_Scheduler::ptr scheduler)
            :Tcp_Client(scheduler)
        {

        }

        Net_Awaiter<int> recv(char* buffer,int len) = delete;
        Net_Awaiter<int> send(const std::string &buffer,uint32_t len) = delete;


        Net_Awaiter<int> request(Http_Request::ptr request,Http_Response::ptr response)
        {
            typename Http_Request_Task<int>::ptr task = std::make_shared<Http_Request_Task<int>>(this->m_fd,this->m_scheduler->m_engine,request,response);
            Tcp_Client::add_task(task);
            return Net_Awaiter<int>{task};
        }

    };
};


#endif