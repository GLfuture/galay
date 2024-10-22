#include "../galay/galay.h"
#include <iostream>
#include <thread>
#include <spdlog/spdlog.h>

uint16_t g_port = 8080;
galay::coroutine::Coroutine test(galay::event::EventEngine* engine, std::vector<galay::async::AsyncTcpSocket>& sockets, int begin, int end)
{
    int64_t start = galay::time::GetCurrentTime();
    for ( int i = begin; i < end; ++ i)
    {
        galay::async::AsyncTcpSocket socket = sockets[i];
        galay::event::NetWaitEvent event(engine, &socket);
        galay::action::NetIoEventAction action(&event);
        galay::async::NetAddr addr{
            .m_ip = "127.0.0.1",
            .m_port = g_port
        };
        bool success = co_await socket.Connect(&action, addr);
        if (!success)
        {
            std::cout << "connect failed" << std::endl;
            success = co_await socket.Close(&action);
            break;
        }
        socket.SetWBuffer("GET / HTTP/1.1\r\nContent-Length: 0\r\n\r\n");
        int length = co_await socket.Send(&action);
        std::cout << "send length: " << length << std::endl;
        length = co_await socket.Recv(&action);
        std::cout << "recv length: " << length << ", " << socket.GetRBuffer() << '\n';
        if(length > 0) delete[] socket.GetRBuffer().cbegin();
        socket.SetRBuffer("");
        bool res = co_await socket.Close(&action);
        if( res ){
            std::cout << "close\n";
        }
    }
    int64_t finish = galay::time::GetCurrentTime();
    std::cout << "end Time: " << finish - start << std::endl;
    co_return;
}

galay::coroutine::Coroutine initSocket(galay::async::AsyncTcpSocket& socket)
{
    galay::event::NetWaitEvent event(nullptr, &socket);
    galay::action::NetIoEventAction action(&event);
    bool res = co_await socket.InitialHandle(&action);
    socket.GetOption().HandleNonBlock();
}

int main(int argc, char* argv[])
{
    if(argc != 2) {
        std::cout << "./tutorial_client [port]\n";
        return -1;
    }
    g_port = atoi(argv[1]);
    galay::scheduler::DynamicResizeCoroutineSchedulers(1);
    galay::scheduler::StartCoroutineSchedulers();
    galay::scheduler::DynamicResizeNetIOSchedulers(1);
    galay::scheduler::StartNetIOSchedulers();
    
    std::vector<galay::async::AsyncTcpSocket> sockets;
    for (size_t i = 0; i < 1024; ++i)
    {
        galay::async::AsyncTcpSocket socket;
        initSocket(socket);
        sockets.push_back(socket);
    }
    std::vector<std::thread> ths;
    for( int i = 0 ; i < 4 ; ++i )
    {
        ths.push_back(std::thread(std::bind(test, galay::scheduler::GetNetIOScheduler(0)->GetEngine(), std::ref(sockets), i * 128, (i + 1) * 128)));
        ths[i].detach();
    }
    getchar();
    galay::scheduler::StopCoroutineSchedulers();
    galay::scheduler::StopNetIOSchedulers();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return 0;
}