#include "galay/galay.h"

using galay::protocol::http::HttpStatusCode;
using galay::protocol::http::HttpVersion;   

class Handler {
public:
    static galay::Coroutine<void> GetHelloWorldHandler(galay::RoutineCtx ctx, galay::HttpSession session) {
        galay::helper::http::HttpHelper::DefaultHttpResponse(session.GetResponse(), HttpVersion::Http_Version_1_1 , HttpStatusCode::OK_200, "text/html", "<html> <h1> Hello World </h1> </html>");
        session.GetResponse()->Header()->HeaderPairs().AddHeaderPair("Connection", "close");
        co_return;
    }

};

int main()
{
    galay::server::HttpServerConfig::ptr config = std::make_shared<galay::server::HttpServerConfig>();
    galay::InitializeGalayEnv(config->m_coroutineConf, config->m_netSchedulerConf, config->m_timerSchedulerConf);
    galay::server::HttpServer<galay::AsyncTcpSocket> server(config);
    server.RouteHandler<galay::HTTPGET>("/", Handler::GetHelloWorldHandler);
    server.Start({"", 8060});
    getchar();
    server.Stop();
    galay::DestroyGalayEnv();
    return 0;
}