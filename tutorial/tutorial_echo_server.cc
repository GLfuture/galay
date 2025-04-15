#include "galay/galay.hpp"

class Handler {
public:
    static galay::Coroutine<void> GetHelloWorldHandler(galay::RoutineCtx ctx, galay::http::HttpContext context) {
        bool res = co_await context.GetStream()->SendResponse(ctx, galay::http::HttpStatusCode::OK_200, "Hello World", "text/plain");
        co_await context.GetStream()->Close();
        co_return;
    }

    static galay::Coroutine<void> GetKeyHandler(galay::RoutineCtx ctx, galay::http::HttpContext context) {
        bool res = co_await context.GetStream()->SendResponse(ctx, galay::http::HttpStatusCode::OK_200, context.GetParam("key"), "text/plain");
        co_await context.GetStream()->Close();
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
    server.RouteHandler<galay::http::GET>("/params/{key}", Handler::GetKeyHandler);
    server.Start({"", port});
    getchar();
    server.Stop();
    return 0;
}