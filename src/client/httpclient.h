#ifndef GALAY_HTTP_CLIENT_H
#define GALAY_HTTP_CLIENT_H

#include "tcpclient.h"

namespace galay
{
    template<Request REQ,Response RESP>
    class Http_Client: public Tcp_Client<REQ,RESP>
    {
    public:
        using ptr = std::shared_ptr<Http_Client>;
        Http_Client(IO_Scheduler<REQ,RESP>::ptr scheduler)
            :Tcp_Client<REQ,RESP>(scheduler)
        {

        }

        Net_Awaiter<REQ,RESP,int> recv(char* buffer,int len) = delete;
        Net_Awaiter<REQ,RESP,int> send(const std::string &buffer,uint32_t len) = delete;


        Net_Awaiter<REQ,RESP,int> request(Http_Request::ptr request,Http_Response::ptr response,int len)
        {
            typename Http_Request_Task<REQ,RESP,int>::ptr task = std::make_shared<Http_Request_Task<REQ,RESP>>(this->m_fd,this->m_scheduler->m_engine,request,response,len);
            Tcp_Client<REQ,RESP>::add_task(task);
            return Net_Awaiter<REQ,RESP,int>{task};
        }

    };
};


#endif