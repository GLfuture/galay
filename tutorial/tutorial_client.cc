#include "galay/galay.h"
#include <iostream>
#include <thread>
#include <spdlog/spdlog.h>

uint16_t g_port = 8080;
galay::coroutine::Coroutine test(galay::event::EventEngine* engine, std::vector<galay::async::AsyncTcpSocket*>& sockets, int begin, int end)
{
    int64_t start = galay::time::GetCurrentTime();
    int i = 0;
    for (i = begin; i < end; ++ i)
    {
        galay::async::AsyncTcpSocket* socket = sockets[i]; 
        galay::event::TcpWaitEvent* event = new galay::event::TcpWaitEvent(socket);
        galay::action::TcpEventAction action(engine, event);
        galay::async::NetAddr addr{
            .m_ip = "127.0.0.1",
            .m_port = g_port
        };
        bool success = co_await socket->Connect(&action, &addr);
        if (!success)
        {
            std::cout << "connect failed" << std::endl;
            success = co_await socket->Close(&action);
            break;
        }
        socket->SetWBuffer("GET / HTTP/1.1\r\nContent-Length: 0\r\n\r\n");
        int length = co_await socket->Send(&action);
        length = co_await socket->Recv(&action);
        if(length > 0) delete[] socket->GetRBuffer().cbegin();
        socket->SetRBuffer("");
        bool res = co_await socket->Close(&action);
    }
    int64_t finish = galay::time::GetCurrentTime();
    std::cout << i - begin << " requests in " << finish - start << " ms" << std::endl;
    co_return;
}

galay::async::AsyncTcpSocket* initSocket()
{
    GHandle handle = galay::async::AsyncTcpSocket::Socket();
    galay::async::AsyncTcpSocket* socket = new galay::async::AsyncTcpSocket(handle);
    socket->GetOption().HandleNonBlock();
    return socket;
}

int main(int argc, char* argv[])
{
    if(argc != 2) {
        std::cout << "./tutorial_client [port]\n";
        return -1;
    }
    spdlog::set_level(spdlog::level::debug);
    g_port = atoi(argv[1]);
    galay::DynamicResizeCoroutineSchedulers(1);
    galay::StartAllCoroutineSchedulers();
    galay::DynamicResizeEventSchedulers(1);
    galay::StartAllEventSchedulers();
    
    std::vector<galay::async::AsyncTcpSocket*> sockets;
    for (size_t i = 0; i < 2048; ++i)
    {
        galay::async::AsyncTcpSocket* socket = initSocket();
        sockets.push_back(socket);
    }
    std::vector<std::thread> ths;
    for( int i = 0 ; i < 8 ; ++i )
    {
        ths.push_back(std::thread(std::bind(test, galay::GetEventScheduler(0)->GetEngine(), std::ref(sockets), i * 256, (i + 1) * 256)));
        ths[i].detach();
    }
    getchar();
    galay::StopAllCoroutineSchedulers();
    galay::StopAllEventSchedulers();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return 0;
}