
# Galay

![Static Badge](https://img.shields.io/badge/License-MIT-yellow)
![Static Badge](https://img.shields.io/badge/Language-C%2B%2B-red%20) 
![Static Badge](https://img.shields.io/badge/Platfrom-Linux%20Mac-red)
![Static Badge](https://img.shields.io/badge/Architecture-x86%20x64-8A2BE2)

## 简介

> galay框架，异步协程网络框架，旨在实现高性能网络IO通信，能够快速搭建HttpServer，避免“回调地狱”，支持Linux和Mac平台，同时内部内置诸多功能库，如命令行解析库，加解密库，ETCD/MySql/Redis客户端库，限流器/熔断器等。

## 目录结构

- [galay(源码主目录)](doc/galay/1.galay.md)
    - [common](doc/galay/2.common.md)
    - [kernel](doc/galay/3.kernel.md)
    - [middleware](doc/galay/4.middleware.md)
    - [protocol](doc/galay/5.protocol.md)
    - [security](doc/galay/6.security.md)
    - [utils](doc/galay/7.utils.md)

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

1. 快速搭建一个带命令行参数的高性能http服务器
``` c++
#include "galay/galay.hpp"

class Handler {
public:
    static galay::Coroutine<void> GetHelloWorldHandler(galay::RoutineCtx ctx, galay::http::HttpStreamImpl<galay::AsyncTcpSocket>::ptr stream) {
        auto& req = stream->GetRequest();
        bool res = co_await stream->SendResponse(ctx, galay::http::HttpStatusCode::OK_200, "Hello World", "text/plain");
        co_await stream->Close();
        co_return;
    }
};

int main(int argc, const char* argv[])
{
    galay::args::App app("tutorial_echo_server");
    app.Help("./tutorial_echo_server [OPTION] COMMAND\nOptions:\n  -p,\t--port number\tremote port.");
    galay::args::Arg::ptr port_arg = galay::args::Arg::Create("port");
    port_arg->Short('p').Required(true).Input(true).IsInt();
    app.AddArg(port_arg);
    if(!app.Parse(argc, argv)) {
        app.ShowHelp();
        return -1;
    }
    uint32_t port;
    try
    {
        port = port_arg->Value().ConvertTo<uint32_t>();
    }
    catch(const std::exception& e)
    {
        return -1;
    }
    galay::GalayEnv env({});
    auto config = galay::http::HttpServerConfig::Create();
    galay::http::HttpServer<galay::AsyncTcpSocket> server(config);
    server.RouteHandler<galay::http::GET>("/hello", Handler::GetHelloWorldHandler);
    server.Start({"", port});
    getchar();
    server.Stop();
    return 0;
}
```
- 运行示例
``` bash
./tutorial_echo_server -p 8080
```

2. 快速搭建静态页面服务器

``` c++
#include "galay/galay.hpp"

int main(int argc, const char* argv[])
{
    galay::args::App app("tutorial_static_server");
    app.Help("./tutorial_static_server [OPTION] COMMAND\nOptions:\n  -p,\t--port number\tremote port.");
    galay::args::Arg::ptr port_arg = galay::args::Arg::Create("port");
    port_arg->Short('p').Required(true).Input(true).IsInt();
    app.AddArg(port_arg);
    if(!app.Parse(argc, argv)) {
        app.ShowHelp();
        return -1;
    }
    uint32_t port;
    try
    {
        port = port_arg->Value().ConvertTo<uint32_t>();
    }
    catch(const std::exception& e)
    {
        return -1;
    }
    galay::GalayEnv env({});
    auto config = galay::http::HttpServerConfig::Create();
    galay::http::HttpServer<galay::AsyncTcpSocket> server(config);
    server.RegisterStaticFileGetMiddleware("/static", "/home/gong/projects/static");
    server.Start({"", port});
    getchar();
    server.Stop();
    return 0;
}
```

- 运行示例

``` bash
./tutorial_static_server -p 8080
```