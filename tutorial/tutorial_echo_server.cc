#include "galay/galay.h"

using galay::protocol::http::HttpStatusCode;
using galay::protocol::http::HttpVersion;   

class Handler {
public:
    static galay::coroutine::Coroutine GetHelloWorldHandler(galay::HttpConnectionManager manager, galay::coroutine::RoutineContext::ptr context) {
        co_await context->DeferDone();
        galay::helper::http::HttpHelper::DefaultHttpResponse(manager.GetResponse(), HttpVersion::Http_Version_1_1 , HttpStatusCode::OK_200, "text/html", "<html> <h1> Hello World </h1> </html>");
        manager.GetResponse()->Header()->HeaderPairs().AddHeaderPair("Connection", "close");
        co_return;
    }

};

int main()
{
    galay::server::HttpServerConfig::ptr config = std::make_shared<galay::server::HttpServerConfig>();
    galay::server::HttpServer server(config);
    server.Get("/", Handler::GetHelloWorldHandler);
    server.Start(8060);
    getchar();
    server.Stop();
    return 0;
}