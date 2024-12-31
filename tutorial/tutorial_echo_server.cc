#include "galay/galay.hpp"

using galay::http::HttpStatusCode;
using galay::http::HttpVersion;   

class Handler {
public:
    static galay::Coroutine<void> GetHelloWorldHandler(galay::RoutineCtx ctx, galay::HttpSession session) {
        auto resp = session.GetResponse();
        resp->Header()->HeaderPairs().AddHeaderPair("Connection", "close");
        resp->SetContent("html", "<html>Hello World</html>");
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
    server.Stop();
    return 0;
}