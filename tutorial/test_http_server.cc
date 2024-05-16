#include "../galay/factory/factory.h"
#include "../galay/kernel/callback.h"
#include <signal.h>
using namespace galay;


GY_TcpCoroutine<> func(Task_Base::wptr t_task)
{
    auto task = t_task.lock();
    auto req = ::std::dynamic_pointer_cast<protocol::Http1_1_Request>(task->GetReq());
    auto resp = ::std::dynamic_pointer_cast<protocol::Http1_1_Response>(task->GetResp());
    ::std::cout << req->GetBody() <<'\n';
    if(task->GetScheduler() == nullptr) ::std::cout<<"NULL\n";
    auto client = Client_Factory::create_http_client(task->GetScheduler());
    auto ret = co_await client->connect("39.156.66.14",80);
    if(client->GetError() == Error::GY_SUCCESS) ::std::cout<<"connect success\n";
    else ::std::cout<<"connect failed error is "<<client->GetError()<<'\n';
    //::std::cout<<req->EncodePdu()<<'\n';
    auto t_req = ::std::make_shared<protocol::Http1_1_Request>();
    t_req->GetVersion() = "1.1";
    t_req->GetMethod() = "GET";
    t_req->GetUri() = "/";
    t_req->SetHeadPair({"Connection","close"});
    ret = co_await client->request(t_req,resp);
    if(client->GetError() == Error::GY_SUCCESS) ::std::cout<<"request success\n";
    else ::std::cout<<"request failed error is "<<client->GetError()<<'\n';
    task->CntlTaskBehavior(Task_Status::GY_TASK_WRITE);
    // if(req->GetHeadValue("Connection").compare("close") == 0) {
    //     ::std::cout << "close\n";
    // }
    task->Finish(); 
    co_return;
}

HttpServer http_server;

void sig_handle(int sig)
{
    http_server->Stop();
}

int main()
{
    signal(SIGINT,sig_handle);
    Callback_ConnClose::set([](int fd){
        ::std::cout << "exit :" << fd << "\n";  
    });
    auto config = Config_Factory::create_http_server_config(Engine_Type::ENGINE_EPOLL,5,3); //5s断
    config->SetConnTimeout(5000);     //5s断
    http_server = Server_Factory::create_http_server(config);
    http_server->Start({{8010,func}});
    return 0;
}