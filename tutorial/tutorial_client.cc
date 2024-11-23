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
        galay::async::NetAddr addr{
            .m_ip = "127.0.0.1",
            .m_port = g_port
        };
        bool success = co_await galay::async::AsyncConnect(socket, &addr);
        if (!success)
        {
            std::cout << "connect failed" << std::endl;
            bool res = co_await galay::async::AsyncClose(socket);
            break;
        }
        
        std::string resp = "GET / HTTP/1.1\r\nContent-Length: 0\r\n\r\n";
        galay::async::IOVec iov {
            .m_buf = resp.data(),
            .m_len = resp.length()
        };
        galay::async::IOVec iov2 {
            .m_buf = new char[1024],
            .m_len = 1024
        };
        int length = co_await galay::async::AsyncSend(socket, &iov);
        length = co_await galay::async::AsyncSend(socket, &iov2);
        delete[] iov2.m_buf;
        bool res = co_await galay::async::AsyncClose(socket);
    }
    int64_t finish = galay::time::GetCurrentTime();
    std::cout << i - begin << " requests in " << finish - start << " ms" << std::endl;
    co_return;
}

galay::async::AsyncTcpSocket* initSocket()
{
    galay::async::AsyncTcpSocket* socket = new galay::async::AsyncTcpSocket(galay::GetEventScheduler(0)->GetEngine());
    galay::async::AsyncSocket(socket);
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
    galay::StartCoroutineSchedulers();
    galay::DynamicResizeEventSchedulers(1);
    galay::StartEventSchedulers(-1);
    
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
    galay::StopCoroutineSchedulers();
    galay::StopEventSchedulers();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return 0;
}