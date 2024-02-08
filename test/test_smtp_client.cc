#include "../galay/factory/factory.h"
#include <signal.h>

using namespace galay;

#define IP "117.135.207.210"

Task<> func(Scheduler_Base::wptr scheduler)
{
    auto smtp_client = Client_Factory::create_smtp_client(scheduler);
    int ret = co_await smtp_client->connect(IP,25);
    if(ret == 0) std::cout<<"connect success\n";
    auto request = std::make_shared<protocol::Smtp_Request>();
    auto response = std::make_shared<protocol::Smtp_Response>();
    ret = co_await smtp_client->request(request->hello(),response->resp());
    if(ret != 0) std::cout<<response->resp()->get_content();
    ret = co_await smtp_client->request(request->auth(),response->resp());
    if(ret != 0) std::cout<<response->resp()->get_content();
    ret = co_await smtp_client->request(request->account("bigdata_C1004@163.com"),response->resp());
    if(ret != 0) std::cout<<response->resp()->get_content();
    ret = co_await smtp_client->request(request->password("EPOXVZMINXCXHXUO"),response->resp());
    if(ret != 0) std::cout<<response->resp()->get_content();
    ret = co_await smtp_client->request(request->mailfrom("bigdata_C1004@163.com"),response->resp());
    if(ret != 0) std::cout<<response->resp()->get_content();
    ret = co_await smtp_client->request(request->rcptto("3370978221@qq.com"),response->resp());
    if(ret != 0) std::cout<<response->resp()->get_content();
    ret = co_await smtp_client->request(request->data(),response->resp());
    if(ret != 0) std::cout<<response->resp()->get_content();
    ret = co_await smtp_client->request(request->msg("Verify","Hello,Baby,到点了还不找我打电话\n"),response->resp());
    if(ret != 0) std::cout<<response->resp()->get_content();
    ret = co_await smtp_client->request(request->quit(),response->resp());
    if(ret != 0) std::cout<<response->resp()->get_content();
    std::cout << "finish\n";
    co_return;
}

Scheduler_Base::ptr scheduler;

void sig_handle(int sig)
{
    scheduler->stop();
}



int main()
{
    signal(SIGINT,sig_handle);
    scheduler = Scheduler_Factory::create_epoll_scheduler(1024,5);
    Task<> co_task = func(scheduler);
    scheduler->start();
    return 0;
}