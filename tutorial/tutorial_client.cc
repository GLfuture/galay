#include "galay/galay.h"
#include <iostream>
#include <thread>
#include <spdlog/spdlog.h>

uint16_t g_port = 8080;
galay::coroutine::Coroutine test(galay::event::EventEngine* engine, std::vector<galay::async::AsyncNetIo*>& sockets, int begin, int end)
{
    int64_t start = galay::GetCurrentTime();
    int i = 0;
    for (i = begin; i < end; ++ i)
    {
        galay::async::AsyncNetIo* socket = sockets[i]; 
        galay::NetAddr addr{
            .m_ip = "127.0.0.1",
            .m_port = g_port
        };
        bool success = co_await galay::AsyncConnect(socket, &addr);
        if (!success)
        {
            std::cout << "connect failed" << std::endl;
            bool res = co_await galay::AsyncClose(socket);
            break;
        }
        
        std::string resp = "GET / HTTP/1.1\r\nContent-Length: 0\r\n\r\n";
        galay::IOVecHolder wholder, rholder(1024);
        wholder.Reset(std::move(resp));
        int length = co_await galay::AsyncSend(socket, &wholder, wholder->m_size);
        length = co_await galay::AsyncRecv(socket, &rholder, rholder->m_size);
        bool res = co_await galay::AsyncClose(socket);
        if (!res)
        {
            std::cout << "close failed" << std::endl;
        }
    }
    int64_t finish = galay::GetCurrentTime();
    std::cout << i - begin << " requests in " << finish - start << " ms" << std::endl;
    co_return;
}

void pack(galay::event::EventEngine* engine, std::vector<galay::async::AsyncNetIo*>& sockets, int begin, int end)
{
    test(engine, sockets, begin, end);
}

galay::async::AsyncNetIo* initSocket()
{
    galay::async::AsyncNetIo* socket = new galay::async::AsyncNetIo(galay::GetEventScheduler(0)->GetEngine());
    galay::AsyncSocket(socket);
    socket->GetOption().HandleNonBlock();
    return socket;
}

int main(int argc, char* argv[])
{
    if(argc != 2) {
        std::cout << "./tutorial_client [port]\n";
        return -1;
    }
    g_port = atoi(argv[1]);
    galay::InitializeGalayEnv(1, 1, 0, -1);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::vector<galay::async::AsyncNetIo*> sockets;
    for (size_t i = 0; i < 2048; ++i)
    {
        galay::async::AsyncNetIo* socket = initSocket();
        sockets.push_back(socket);
    }
    std::vector<std::thread> ths;
    for( int i = 0 ; i < 8 ; ++i )
    {
        ths.push_back(std::thread(std::bind(pack, galay::GetEventScheduler(0)->GetEngine(), std::ref(sockets), i * 256, (i + 1) * 256)));
        ths[i].detach();
    }
    getchar();
    galay::DestroyGalayEnv();
    return 0;
}