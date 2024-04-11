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
    ret = co_await smtp_client->request(request->Hello(),response->Resp());
    if(std::any_cast<int>(ret) != 0) std::cout<<response->Resp()->GetContent();
    ret = co_await smtp_client->request(request->Auth(),response->Resp());
    if(std::any_cast<int>(ret) != 0) std::cout<<response->Resp()->GetContent();
    ret = co_await smtp_client->request(request->Account("bigdata_C1004@163.com"),response->Resp());
    if(std::any_cast<int>(ret) != 0) std::cout<<response->Resp()->GetContent();
    ret = co_await smtp_client->request(request->Password("EPOXVZMINXCXHXUO"),response->Resp());
    if(std::any_cast<int>(ret) != 0) std::cout<<response->Resp()->GetContent();
    ret = co_await smtp_client->request(request->MailFrom("bigdata_C1004@163.com"),response->Resp());
    if(std::any_cast<int>(ret) != 0) std::cout<<response->Resp()->GetContent();
    ret = co_await smtp_client->request(request->RcptTo("123@qq.com"),response->Resp());
    if(std::any_cast<int>(ret) != 0) std::cout<<response->Resp()->GetContent();
    ret = co_await smtp_client->request(request->Data(),response->Resp());
    if(std::any_cast<int>(ret) != 0) std::cout<<response->Resp()->GetContent();
    ret = co_await smtp_client->request(request->Msg("Verify","It is a demo\n"),response->Resp());
    if(std::any_cast<int>(ret) != 0) std::cout<<response->Resp()->GetContent();
    ret = co_await smtp_client->request(request->Quit(),response->Resp());
    if(std::any_cast<int>(ret) != 0) std::cout<<response->Resp()->GetContent();
    co_return;
}

Scheduler_Base::ptr scheduler;

void sig_handle(int sig)
{
    scheduler->Stop();
}



int main()
{
    signal(SIGINT,sig_handle);
    scheduler = Scheduler_Factory::create_epoll_scheduler(1024,5);
    Task<> co_task = func(scheduler);
    scheduler->Start();
    return 0;
}