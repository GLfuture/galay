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
    galay::server::TcpSslServerConfig::ptr config = std::make_shared<galay::server::TcpSslServerConfig>();
    config->m_cert_file = argv[1];
    config->m_key_file = argv[2];
    galay::server::TcpSslServer server(config);
    galay::TcpSslCallbackStore store([](galay::TcpSslOperation op)->galay::coroutine::Coroutine
    {
        
        co_return;
    });
    server.Start(&store, 2333);
    getchar();
    server.Stop();
    return 0;
}