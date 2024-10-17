#ifndef __GALAY_SERVER_H__
#define __GALAY_SERVER_H__

#include <string>
#include <memory>
#include <vector>

namespace galay::event
{
    class ListenEvent;
};

namespace galay::async
{
    class AsyncTcpSocket;
}

namespace galay
{
    class CallbackStore;
}

namespace galay::server 
{

#define DEFAULT_SERVER_NET_SCHEDULER_NUM 4
#define DEFAULT_SERVER_CO_SCHEDULER_NUM 4
    
    class TcpServer
    {
    public:
        TcpServer();
        //no block
        void Start(CallbackStore* store, int port, int backlog);
        void Stop();
        void ReSetNetworkSchedulerNum(int num);
        void ReSetCoroutineSchedulerNum(int num);
        async::AsyncTcpSocket* GetSocket();
        ~TcpServer();
    private:
        int m_co_sche_num;
        int m_net_sche_num;
        int m_co_sche_timeout;
        int m_net_sche_timeout;
        async::AsyncTcpSocket* m_socket;
        std::vector<event::ListenEvent*> m_listen_events;
    };
}

#endif