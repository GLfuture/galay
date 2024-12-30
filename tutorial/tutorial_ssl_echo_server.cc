#include "galay/galay.h"
#include <spdlog/spdlog.h>
#include <iostream>

int main(int argc, char** argv)
{
    if(argc < 3) {
        std::cout << "Usage: " << argv[0] << " <certificate.crt> <private.key>" << std::endl;
        return -1;
    }
    spdlog::set_level(spdlog::level::debug);
    galay::server::HttpServerConfig::ptr config = std::make_shared<galay::server::HttpServerConfig>();
    galay::InitializeGalayEnv(config->m_coroutineConf, config->m_netSchedulerConf, config->m_timerSchedulerConf);
    galay::InitializeSSLServerEnv(argv[1], argv[2]);
    galay::server::HttpServer<galay::AsyncTcpSslSocket> server(config);
    server.Start({"", 2333});
    getchar();
    server.Stop();
    galay::DestroySSLEnv();
    galay::DestroyGalayEnv();
    return 0;
}