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

namespace galay::coroutine
{
    class Coroutine;
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
        coroutine::Coroutine Start(CallbackStore* store, int port, int backlog);
        void Stop();
        inline bool IsRunning() { return m_is_running; }
        void ReSetNetworkSchedulerNum(int num);
        void ReSetCoroutineSchedulerNum(int num);
        async::AsyncTcpSocket* GetSocket();
        ~TcpServer();
    private:
        int m_co_sche_num;
        int m_net_sche_num;
        int m_co_sche_timeout;
        int m_net_sche_timeout;
        bool m_is_running;
        async::AsyncTcpSocket* m_socket;
        std::vector<event::ListenEvent*> m_listen_events;
    };
}

#endif