
# Galay

![Static Badge](https://img.shields.io/badge/License-MIT-yellow)
![Static Badge](https://img.shields.io/badge/Language-C%2B%2B-red%20) 
![Static Badge](https://img.shields.io/badge/Platfrom-Linux%20Mac-red)
![Static Badge](https://img.shields.io/badge/Architecture-x86%20x64-8A2BE2)

## 简介

> galay框架，异步协程框架

## 安装

1. 下载必要依赖openssl，g++(version > 9)，cmake，spdlog，libaio（linux下）
    >```shell
    > sudo apt-get install libssl-dev g++ cmake libaio 
    > git clone https://github.com/gabime/spdlog.git
    > cd spdlog
    > mkdir build
    > cd build && sudo make -j4 && make install
    >```

2. 编译安装
    > ```shell
    > mkdir build
    > cd build
    > cmake ..
    > make -j4 && sudo make install
    >```
    > 头文件安装在/user/local/include,库文件安装在/user/local/lib

3. 使用
    > ```shell
    > g++ example.cc -o example -lgalay
    > # g++ example.cc -o example -lgalay-static
    > ```
    > 具体使用请查看tutorial目录下的内容

## 使用

1. 快速搭建一个高性能http服务器
``` c++
#include "galay/galay.hpp"

class Handler {
public:
    static galay::Coroutine<void> GetHelloWorldHandler(galay::RoutineCtx ctx, galay::HttpSession::ptr session) {
        auto resp = session->GetResponse();
        resp->SetContent("html", "<html>Hello World</html>");
        session->Close();
        co_return;
    }
};

int main(int argc, char* argv[])
{
    galay::GalayEnv env({});
    auto config = galay::server::HttpServerConfig::Create();
    galay::server::HttpServer<galay::AsyncTcpSocket> server(config);
    server.RouteHandler<galay::http::GET>("/", Handler::GetHelloWorldHandler);
    server.Start({"", 8060});
    getchar();
    server.Stop();
    return 0;
}
```