#include "galay/galay.hpp"
#include <iostream>
#include <thread>
#include <spdlog/spdlog.h>

uint32_t g_port = 8080;
galay::Coroutine<void> test(galay::RoutineCtx::ptr ctx, galay::details::EventEngine* engine, std::vector<galay::AsyncTcpSocket*>& sockets, int begin, int end)
{
    int64_t start = galay::utils::GetCurrentTimeMs();
    int i = 0;
    for (i = begin; i < end; ++ i)
    {
        galay::AsyncTcpSocket* socket = sockets[i]; 
        galay::THost addr{
            .m_ip = "127.0.0.1",
            .m_port = g_port
        };
        bool success = co_await socket->Connect(&addr);
        if (!success)
        {
            std::cout << "connect failed" << std::endl;
            bool res = co_await socket->Close();
            break;
        }
        
        std::string resp = "GET / HTTP/1.1\r\nContent-Length: 0\r\n\r\n";
        galay::IOVecHolder<galay::TcpIOVec> wholder, rholder(1024);
        wholder.Reset(std::move(resp));
        int length = co_await socket->Send(&wholder, wholder->m_size);
        length = co_await socket->Recv(&rholder, rholder->m_size);
        bool res = co_await socket->Close();
        if (!res)
        {
            std::cout << "close failed" << std::endl;
        }
    }
    int64_t finish = galay::utils::GetCurrentTimeMs();
    std::cout << i - begin << " requests in " << finish - start << " ms" << std::endl;
    co_return;
}

void pack(galay::details::EventEngine* engine, std::vector<galay::AsyncTcpSocket*>& sockets, int begin, int end)
{
    test({}, engine, sockets, begin, end);
}

galay::AsyncTcpSocket* initSocket()
{
    galay::AsyncTcpSocket* socket = new galay::AsyncTcpSocket(galay::EventSchedulerHolder::GetInstance()->GetScheduler(0)->GetEngine());
    socket->Socket();
    return socket;
}

int main(int argc, char* argv[])
{
    if(argc != 2) {
        std::cout << "./tutorial_client [port]\n";
        return -1;
    }
    g_port = atoi(argv[1]);
    galay::InitializeGalayEnv({1, -1}, {1, -1}, {1, -1});
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::vector<galay::AsyncTcpSocket*> sockets;
    for (size_t i = 0; i < 2048; ++i)
    {
        galay::AsyncTcpSocket* socket = initSocket();
        sockets.push_back(socket);
    }
    std::vector<std::thread> ths;
    for( int i = 0 ; i < 8 ; ++i )
    {
        ths.push_back(std::thread(std::bind(pack, galay::EventSchedulerHolder::GetInstance()->GetScheduler(0)->GetEngine(), std::ref(sockets), i * 256, (i + 1) * 256)));
        ths[i].detach();
    }
    getchar();
    for( int i = 0 ; i < 2048 ; ++i ) {
        delete sockets[i];
    }
    galay::DestroyGalayEnv();
    return 0;
}