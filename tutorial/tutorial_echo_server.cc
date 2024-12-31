#include "galay/galay.hpp"

using galay::http::HttpStatusCode;
using galay::http::HttpVersion;   

class Handler {
public:
    static galay::Coroutine<void> GetHelloWorldHandler(galay::RoutineCtx ctx, galay::HttpSession session) {
        galay::http::HttpHelper::DefaultHttpResponse(session.GetResponse(), HttpVersion::Http_Version_1_1 , HttpStatusCode::OK_200, "text/html", "<html> <h1> Hello World </h1> </html>");
        session.GetResponse()->Header()->HeaderPairs().AddHeaderPair("Connection", "close");
        co_return;
    }

};

int main()
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