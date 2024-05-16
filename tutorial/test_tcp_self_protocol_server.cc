#include "../galay/protocol/basic_protocol.h"
#include "../galay/factory/factory.h"
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <cstring>
#include <signal.h>

using namespace galay;

struct self_head
{
    char version[4];
    int length;
};

class Self_Request: public GY_TcpRequest
{
public:
    using ptr = ::std::shared_ptr<Self_Request>;
    galay::ProtoJudgeType DecodePdu(const ::std::string& buffer) override
    {
        if(buffer.length() < sizeof(self_head)) {
            spdlog::warn("[{}:{}] [protocol incomplete]",__FILE__,__LINE__);
            return galay::ProtoJudgeType::PROTOCOL_INCOMPLETE;
        }
        if(m_header_len == 0){
            memcpy(&m_head.version,buffer.substr(0,4).c_str(),sizeof(4));
            memcpy(&m_head.length,buffer.substr(4,sizeof(int)).c_str(),sizeof(int));
            m_head.length = ntohl(m_head.length);
            spdlog::info("[{}:{}] [version:{}] [length:{}]",__FILE__,__LINE__,m_head.version,m_head.length);
            m_header_len = sizeof(self_head);
        }
        if(buffer.length() < m_head.length + sizeof(self_head)) {
            spdlog::warn("[{}:{}] [protocol incomplete] buffer len: {} , require: {}",__FILE__,__LINE__,buffer.length(),m_head.length + sizeof(self_head));
            return galay::ProtoJudgeType::PROTOCOL_INCOMPLETE;
        }
        return galay::ProtoJudgeType::PROTOCOL_FINISHED;
    }

    int DecodePdu(::std::string &buffer) override
    {
        m_body = buffer.substr(m_header_len,m_head.length);
        int res = m_header_len+ m_head.length;
        m_header_len = 0;
        m_head.length = 0;
        return res;
    }

    ::std::string EncodePdu()
    {
        char temp[sizeof(self_head)];
        memset(temp,0,sizeof(self_head));
        memcpy(temp,&m_head,sizeof(self_head));
        ::std::string res(temp,sizeof(self_head));
        res += m_body;
        return res;
    }

    ::std::string& GetBody()
    {
        return m_body;
    }

    self_head& GetHead()
    {
        return m_head;
    }

private:
    int m_header_len = 0;
    self_head m_head;
    ::std::string m_body;
};

class Self_Response: public GY_TcpResponse
{
public:
    using ptr =::std::shared_ptr<Self_Response>;
    galay::ProtoJudgeType DecodePdu(const ::std::string& buffer)
    {
        if(buffer.length() < sizeof(self_head)) {
            spdlog::warn("[{}:{}] [protocol incomplete]",__FILE__,__LINE__);
            return galay::ProtoJudgeType::PROTOCOL_INCOMPLETE;
        }
        if(m_header_len == 0){
            memcpy(&m_head.version,buffer.substr(0,4).c_str(),sizeof(4));
            memcpy(&m_head.length,buffer.substr(4,sizeof(int)).c_str(),sizeof(int));
            spdlog::error("[{}:{}] [version:{}] [length:{}]",__FILE__,__LINE__,m_head.version,m_head.length);
            m_head.length = ntohl(m_head.length);
            spdlog::error("[{}:{}] [version:{}] [length:{}]",__FILE__,__LINE__,m_head.version,m_head.length);
            m_header_len = sizeof(self_head);
        }
        if(buffer.length() < m_head.length + sizeof(self_head)) {
            spdlog::warn("[{}:{}] [protocol incomplete]",__FILE__,__LINE__);
            return galay::ProtoJudgeType::PROTOCOL_INCOMPLETE;
        }
        return galay::ProtoJudgeType::PROTOCOL_FINISHED;
    }

    int DecodePdu(::std::string &buffer)
    {
        m_body = buffer.substr(m_header_len,m_head.length);
        int res = m_header_len+ m_head.length;
        m_header_len = 0;
        m_head.length = 0;
        return res;
    }

    ::std::string EncodePdu() override
    {
        char temp[sizeof(self_head)];
        memset(temp,0,sizeof(self_head));
        memcpy(temp,&m_head,sizeof(self_head));
        ::std::string res(temp,sizeof(self_head));
        res += m_body;
        return res;
    }
    ::std::string& GetBody()
    {
        return m_body;
    }

    self_head& GetHead()
    {
        return m_head;
    }

private:
    int m_header_len = 0;
    self_head m_head;
    ::std::string m_body;
};


GY_TcpCoroutine<> func(Task_Base::wptr t_task)
{
    auto req = ::std::dynamic_pointer_cast<Self_Request>(t_task.lock()->GetReq());
    auto resp = ::std::dynamic_pointer_cast<Self_Response>(t_task.lock()->GetResp());
    if(!t_task.lock()->GetContext().has_value()) t_task.lock()->GetContext() = 0;
    else {
        int& ctx = ::std::any_cast<int&>(t_task.lock()->GetContext());
        ctx ++;
    }
    for(int i = 0 ; i < 10000 ; i++){}
    // if(::std::any_cast<int>(t_task.lock()->GetContext()) % 1000 == 0) {
    //     ::std::cout<< ::std::any_cast<int>(t_task.lock()->GetContext()) << " " << req->GetBody() <<'\n';
    // }
    resp->GetBody() = req->GetBody();
    req->GetHead().length = htonl(req->GetHead().length);
    resp->GetHead() = req->GetHead();
    t_task.lock()->CntlTaskBehavior(Task_Status::GY_TASK_WRITE);
    return {};
}

Tcp_Server<Self_Request,Self_Response>::uptr server;

void sig_handle(int sig)
{
    server->Stop();
}


int main()
{
    signal(SIGINT,sig_handle);
    spdlog::flush_on(spdlog::level::info);
    spdlog::init_thread_pool(8192, 1);
    auto rotating_sink = ::std::make_shared<spdlog::sinks::rotating_file_sink_mt>("server.log", 1024*1024*10, 3);
    ::std::vector<spdlog::sink_ptr> sinks {rotating_sink};
    auto logger = ::std::make_shared<spdlog::async_logger>("loggername", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
    spdlog::set_default_logger(logger);
    auto config = Config_Factory::create_tcp_server_config(Engine_Type::ENGINE_EPOLL,5,1);
    //config->SetConnTimeout(5000);
    config->SetKeepalive(10,3,3);
    server = ::std::make_unique<Tcp_Server<Self_Request,Self_Response>>(config);
    server->Start({{8010,func},{8011,func}});
    return 0;
}