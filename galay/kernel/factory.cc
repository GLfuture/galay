#include "factory.h"
#include "builder.h"
#include "router.h"
#include "server.h"


std::shared_ptr<galay::kernel::GY_HttpRouter> 
galay::GY_RouterFactory::CreateHttpRouter()
{
    return std::make_shared<kernel::GY_HttpRouter>();
}


std::shared_ptr<galay::kernel::GY_HttpServerBuilder> 
galay::GY_ServerBuilderFactory::CreateHttpServerBuilder(
    int port,std::shared_ptr<kernel::GY_HttpRouter> router,common::SchedulerType type,uint16_t thread_num)
{
    auto builder = std::make_shared<kernel::GY_HttpServerBuilder>();
    builder->SetPort(port);
    builder->SetRouter(router);
    builder->SetSchedulerType(type);
    builder->SetNetThreadNum(thread_num);
    return builder;
}

std::shared_ptr<galay::kernel::GY_HttpServerBuilder> 
galay::GY_ServerBuilderFactory::CreateHttpsServerBuilder(
    const std::string& key_file,const std::string& cert_file, int port,
    std::shared_ptr<kernel::GY_HttpRouter> router,common::SchedulerType type,uint16_t thread_num)
{
    auto builder = std::make_shared<kernel::GY_HttpServerBuilder>();
    builder->SetPort(port);
    builder->SetRouter(router);
    builder->SetSchedulerType(type);
    builder->SetNetThreadNum(thread_num);
    builder->InitSSLServer(true);
    builder->GetSSLConfig()->SetCertPath(cert_file);
    builder->GetSSLConfig()->SetKeyPath(key_file);
    return builder;
}

std::shared_ptr<galay::kernel::GY_TcpServer>
galay::GY_ServerFactory::CreateHttpServer(std::shared_ptr<kernel::GY_HttpServerBuilder> builder)
{
    auto server = std::make_shared<kernel::GY_TcpServer>();
    server->SetServerBuilder(builder);
    return server;
}


std::shared_ptr<galay::kernel::GY_TcpServer> 
galay::GY_ServerFactory::CreateHttpsServer(std::shared_ptr<kernel::GY_HttpServerBuilder> builder)
{
    auto server = std::make_shared<kernel::GY_TcpServer>();
    server->SetServerBuilder(builder);
    return server;
}