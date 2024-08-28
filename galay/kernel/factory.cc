#include "factory.h"
#include "builder.h"
#include "router.h"
#include "server.h"

namespace galay
{
std::shared_ptr<server::HttpRouter> 
galay::RouterFactory::CreateHttpRouter()
{
    return std::make_shared<server::HttpRouter>();
}


std::shared_ptr<server::HttpServerBuilder> 
galay::ServerBuilderFactory::CreateHttpServerBuilder(
    int port,std::shared_ptr<server::HttpRouter> router,server::PollerType type,uint16_t thread_num)
{
    auto builder = std::make_shared<server::HttpServerBuilder>();
    builder->SetPort(port);
    builder->SetRouter(router);
    builder->SetPollerType(type);
    builder->SetNetThreadNum(thread_num);
    return builder;
}

std::shared_ptr<server::HttpServerBuilder> 
galay::ServerBuilderFactory::CreateHttpsServerBuilder(
    const std::string& key_file,const std::string& cert_file, int port,
    std::shared_ptr<server::HttpRouter> router,server::PollerType type,uint16_t thread_num)
{
    auto builder = std::make_shared<server::HttpServerBuilder>();
    builder->SetPort(port);
    builder->SetRouter(router);
    builder->SetPollerType(type);
    builder->SetNetThreadNum(thread_num);
    io::net::SSLConfig::ptr config = std::make_shared<io::net::SSLConfig>();
    config->SetKeyPath(key_file);
    config->SetCertPath(cert_file);
    builder->InitSSLServer(config);
    return builder;
}

std::shared_ptr<server::TcpServer>
galay::ServerFactory::CreateHttpServer(std::shared_ptr<server::HttpServerBuilder> builder)
{
    auto server = std::make_shared<server::TcpServer>();
    server->SetServerBuilder(builder);
    return server;
}


std::shared_ptr<server::TcpServer> 
galay::ServerFactory::CreateHttpsServer(std::shared_ptr<server::HttpServerBuilder> builder)
{
    auto server = std::make_shared<server::TcpServer>();
    server->SetServerBuilder(builder);
    return server;
}
}