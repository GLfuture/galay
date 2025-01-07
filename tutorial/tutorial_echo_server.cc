#include "galay/galay.hpp"

class Handler {
public:
    static galay::Coroutine<void> GetHelloWorldHandler(galay::RoutineCtx ctx, galay::HttpSession session) {
        auto resp = session.GetResponse();
        resp->SetContent("html", "<html>Hello World</html>");
        session.ToClose();
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