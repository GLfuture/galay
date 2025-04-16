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
    server.RegisterStaticFileGetMiddleware("/static", "/home/gong/static");
    server.Start({"", port});
    getchar();
    server.Stop();
    return 0;
}