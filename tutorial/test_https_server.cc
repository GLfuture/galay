#include "../galay/factory/factory.h"
#include <signal.h>
using namespace galay;

Task<> func(Task_Base::wptr t_task)
{
    auto task = t_task.lock();
    auto req = std::dynamic_pointer_cast<Protocol::Http1_1_Request>(task->GetReq());
    auto resp = std::dynamic_pointer_cast<Protocol::Http1_1_Response>(task->GetResp());
    std::cout << req->GetBody() <<'\n';
    if(task->GetScheduler() == nullptr) std::cout<<"NULL\n";
    auto client = Client_Factory::create_http_client(task->GetScheduler());
    auto ret = co_await client->connect("39.156.66.14",80);
    if(client->GetError() == Error::GY_SUCCESS) std::cout<<"connect success\n";
    else std::cout<<"connect failed error is "<<client->GetError()<<'\n';
    //std::cout<<req->EncodePdu()<<'\n';
    auto t_req = std::make_shared<Protocol::Http1_1_Request>();
    t_req->GetVersion() = "1.1";
    t_req->GetMethod() = "GET";
    t_req->GetUri() = "/";
    t_req->SetHeadPair({"Connection","close"});
    ret = co_await client->request(t_req,resp);
    if(client->GetError() == Error::GY_SUCCESS) std::cout<<"request success\n";
    else std::cout<<"request failed error is "<<client->GetError()<<'\n';
    task->CntlTaskBehavior(Task_Status::GY_TASK_WRITE);
    // if(req->GetHeadValue("Connection").compare("close") == 0) {
    //     std::cout << "close\n";
    // }
    task->Finish(); 
    co_return;
}

HttpsServer https_server;

void sig_handle(int sig)
{
    https_server->Stop();
}

int main()
{
    signal(SIGINT, sig_handle);
    Callback_ConnClose::set([](int fd){
        std::cout << "exit :" << fd << "\n";  
    });
    auto config = Config_Factory::create_https_server_config(Engine_Type::ENGINE_SELECT,5,5);
    config->SetSSLConf(DEFAULT_SSL_MIN_VERSION,DEFAULT_SSL_MAX_VERSION,5,1,"../server.crt","../server.key");
    config->SetConnTimeout(5000);
    std::cout << config->m_ssl_conf.m_cert_filepath << "\n";
    std::cout << config->m_ssl_conf.m_key_filepath << "\n";
    https_server = Server_Factory::create_https_server(config);
    https_server->Start({{8010,func},{8011,func}});
    return 0;
}