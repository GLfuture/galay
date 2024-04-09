#include "../galay/factory/factory.h"
#include <signal.h>

using namespace galay;

#define IP "117.135.207.210"

Task<> func(Scheduler_Base::wptr scheduler)
{
    auto smtp_client = Client_Factory::create_smtp_client(scheduler);
    auto ret = co_await smtp_client->connect(IP,25);
    if(std::any_cast<int>(ret) == 0) std::cout<<"connect success\n";
    auto request = std::make_shared<Protocol::Smtp_Request>();
    auto response = std::make_shared<Protocol::Smtp_Response>();
    ret = co_await smtp_client->request(request->hello(),response->resp());
    if(std::any_cast<int>(ret) != 0) std::cout<<response->resp()->get_content();
    ret = co_await smtp_client->request(request->auth(),response->resp());
    if(std::any_cast<int>(ret) != 0) std::cout<<response->resp()->get_content();
    ret = co_await smtp_client->request(request->account("bigdata_C1004@163.com"),response->resp());
    if(std::any_cast<int>(ret) != 0) std::cout<<response->resp()->get_content();
    ret = co_await smtp_client->request(request->password("EPOXVZMINXCXHXUO"),response->resp());
    if(std::any_cast<int>(ret) != 0) std::cout<<response->resp()->get_content();
    ret = co_await smtp_client->request(request->mailfrom("bigdata_C1004@163.com"),response->resp());
    if(std::any_cast<int>(ret) != 0) std::cout<<response->resp()->get_content();
    ret = co_await smtp_client->request(request->rcptto("123@qq.com"),response->resp());
    if(std::any_cast<int>(ret) != 0) std::cout<<response->resp()->get_content();
    ret = co_await smtp_client->request(request->data(),response->resp());
    if(std::any_cast<int>(ret) != 0) std::cout<<response->resp()->get_content();
    ret = co_await smtp_client->request(request->msg("Verify","It is a demo\n"),response->resp());
    if(std::any_cast<int>(ret) != 0) std::cout<<response->resp()->get_content();
    ret = co_await smtp_client->request(request->quit(),response->resp());
    if(std::any_cast<int>(ret) != 0) std::cout<<response->resp()->get_content();
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