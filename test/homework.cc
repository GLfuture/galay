#include "../src/factory/factory.h"
#include <nlohmann/json.hpp>
#include <signal.h>
using namespace galay;

Task<> func(Task_Base::wptr t_task)
{
    auto task = t_task.lock();
    auto resp = std::dynamic_pointer_cast<Http_Response>(task->get_resp());
    auto req = std::dynamic_pointer_cast<Http_Request>(task->get_req());
    std::cout<< req->encode();
    // auto client = Client_Factory::create_http_client(task->get_scheduler());
    // int ret = co_await client->connect("192.168.196.239",8080);
    // if(ret == 0) {
    //     std::cout<<"connect success\n";
    // }else{
    //     std::cout<<"connect failed\n";
    // }
    // Http_Request::ptr request = std::dynamic_pointer_cast<Http_Request>(task->get_req());
    // Http_Response::ptr response = std::make_shared<Http_Response>();
    // ret = co_await client->request(request,response);
    // client->disconnect();
    // if(ret != 0) task->finish();
    // nlohmann::json js,ret_js,js_arr1 = nlohmann::json::array(),js_arr2 = nlohmann::json::array();
    // js = nlohmann::json::parse(response->get_body());
    // for(auto &row : js["pred"])
    // {
    //     for(auto &e : row){
    //         js_arr1.push_back(e);
    //     }
    // }
    // for(auto &row : js["true_row"])
    // {
    //     for(auto &e : row){
    //         js_arr2.push_back(e);
    //     }
    // }
    // ret_js["pred"] = js_arr1;
    // ret_js["true_row"] = js_arr2;
    resp->get_version() = "HTTP/1.1";
    resp->get_status() = 200;
    resp->set_head_kv_pair({"Connection","close"});
    resp->get_body() = "hello world";
    //resp->get_body() = ret_js.dump();
    std::cout<<resp->encode();
    task->finish();
    co_return;
}

void sig_handle(int sig)
{
}

int main()
{
    signal(SIGINT,sig_handle);
    auto config = Config_Factory::create_http_server_config(8080);
    auto scheduler = Scheduler_Factory::create_http_scheduler(IO_EPOLL,1024,5);
    auto http_server = Server_Factory::create_http_server(config,scheduler);
    http_server->start(func);
    return 0;
}