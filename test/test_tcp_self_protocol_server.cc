#include "../galay/protocol/basic_protocol.h"
#include "../galay/factory/factory.h"
#include <cstring>
#include <signal.h>

using namespace galay;

struct self_head
{
    char version[4];
    int length;
};

class Self_Request: public Tcp_Request_Base
{
public:
    using ptr = std::shared_ptr<Self_Request>;
    int decode(const std::string &buffer , int &state) override
    {
        memcpy(&m_head.version,buffer.substr(0,4).c_str(),sizeof(4));
        memcpy(&m_head.length,buffer.substr(4,sizeof(int)).c_str(),sizeof(int));
        m_head.length = ntohl(m_head.length);
        return sizeof(self_head);
    }

    int proto_type() override
    {
        return GY_HEAD_FIXED_PROTOCOL_TYPE;
    }

    int proto_fixed_len() override
    {
        return sizeof(self_head);
    }

    int proto_extra_len() override
    {
        return m_head.length;
    }

    void set_extra_msg(std::string &&msg) 
    {
        body = msg;
    }

    std::string& get_body()
    {
        return body;
    }

    self_head& get_head()
    {
        return m_head;
    }

private:
    self_head m_head;
    std::string body;
};

class Self_Response: public Tcp_Response_Base
{
public:
    using ptr =std::shared_ptr<Self_Response>;
    std::string encode()
    {
        char temp[sizeof(self_head)];
        memset(temp,0,sizeof(self_head));
        memcpy(temp,&m_head,sizeof(self_head));
        std::string res(temp,sizeof(self_head));
        res += body;
        return res;
    }

    std::string& get_body()
    {
        return body;
    }

    self_head& get_head()
    {
        return m_head;
    }

private:
    self_head m_head;
    std::string body;
};


Task<> func(Task_Base::wptr t_task)
{
    auto req = std::dynamic_pointer_cast<Self_Request>(t_task.lock()->get_req());
    auto resp = std::dynamic_pointer_cast<Self_Response>(t_task.lock()->get_resp());
    if(!t_task.lock()->get_ctx().has_value()) t_task.lock()->get_ctx() = 0;
    else {
        int& ctx = std::any_cast<int&>(t_task.lock()->get_ctx());
        ctx ++;
    }
    if(std::any_cast<int>(t_task.lock()->get_ctx()) % 1000 == 0) std::cout<< std::any_cast<int>(t_task.lock()->get_ctx()) <<" :  " <<req->get_head().version << " "<<req->get_body()<<'\n';
    t_task.lock()->control_task_behavior(Task_Status::GY_TASK_WRITE);
    resp->get_body() = req->get_body();
    req->get_head().length = htonl(req->get_head().length);
    resp->get_head() = req->get_head();
    return {};
}

Tcp_Server<Self_Request,Self_Response>::ptr server;

void sig_handle(int sig)
{
    server->stop();
}


int main()
{
    signal(SIGINT,sig_handle);
    auto config = Config_Factory::create_tcp_server_config(8080);
    config->enable_keepalive(10,3,3);
    auto scheduler = Scheduler_Factory::create_epoll_scheduler(1024,5);
    server = std::make_shared<Tcp_Server<Self_Request,Self_Response>>(config,scheduler);
    server->start(func);
    return 0;
}