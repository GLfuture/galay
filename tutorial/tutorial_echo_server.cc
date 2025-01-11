#include "galay/galay.hpp"
#include <iostream>

class Handler {
public:
    static galay::Coroutine<void> GetHelloWorldHandler(galay::RoutineCtx ctx, galay::HttpSession::ptr session) {
        std::cout << ctx.GetThisLayer() << std::endl;
        auto& graph = ctx.GetSharedCtx().lock()->GetRoutineGraph();
        for(int i = 0; i < graph.size(); ++i) {
            std::cout << "layer: " << i << std::endl;
            for(auto& coroutine: graph[i]) {
                std::cout << "sequence: " << coroutine.first << " " << std::endl;
            }
        }
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