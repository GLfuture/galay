
#include "../galay/factory/factory.h" 
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <fstream>
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
        if(buffer.length() < sizeof(self_head)) return galay::ProtoJudgeType::PROTOCOL_INCOMPLETE;
        if(m_header_len == 0){
            memcpy(&m_head.version,buffer.substr(0,4).c_str(),sizeof(4));
            memcpy(&m_head.length,buffer.substr(4,sizeof(int)).c_str(),sizeof(int));
            m_head.length = ntohl(m_head.length);
            m_header_len = sizeof(self_head);
        }
        if(buffer.length() < m_head.length + sizeof(self_head)) return galay::ProtoJudgeType::PROTOCOL_INCOMPLETE;
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
        if(buffer.length() < sizeof(self_head)) return galay::ProtoJudgeType::PROTOCOL_INCOMPLETE;
        if(m_header_len == 0){
            memcpy(&m_head.version,buffer.substr(0,4).c_str(),sizeof(4));
            memcpy(&m_head.length,buffer.substr(4,sizeof(int)).c_str(),sizeof(int));
            m_head.length = ntohl(m_head.length);
            m_header_len = sizeof(self_head);
        }
        if(buffer.length() < m_head.length + sizeof(self_head)) return galay::ProtoJudgeType::PROTOCOL_INCOMPLETE;
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

GY_TcpCoroutine<> func(Epoll_Scheduler::ptr scheduler)
{
    auto id = ::std::this_thread::get_id();
    auto client = Client_Factory::create_tcp_self_define_client(scheduler);
    auto ret = co_await client->connect("127.0.0.1",8010);
    if(::std::any_cast<int>(ret) == 0) {
        ::std::cout<< "th :"<< *(unsigned int*)&id <<" connect success\n";
    }else{
        ::std::cout<<"connect failed\n";
        scheduler->Stop();
        co_return;
    }
    auto request = ::std::make_shared<Self_Request>();
    auto response = ::std::make_shared<Self_Response>();
    self_head& head = request->GetHead();
    memcpy(head.version,"1.1",4);
    //::std::ofstream out(::std::to_string(*(unsigned int*)&id));
    for(int i = 0 ; i <= 1000 ; i++)
    {
        ::std::string buffer = ::std::to_string(*(unsigned int*)&id) + ": hello world\n";
        head.length = htonl(buffer.length());
        request->GetBody() = buffer;
        ret = co_await client->request(request,response);
        if(::std::any_cast<int>(ret) == -1) {
            ::std::cout<< *(unsigned int*)&id << " : send fialed\n";
            break;
        }
        // if(i % 1000 == 0){
        //    ::std::cout << i << " : " << response->GetBody() << '\n';
        // }
    }
    scheduler->Stop();
    co_return;
}

#define THREAD_NUM 4

::std::vector<Scheduler_Base::ptr> schedulers;

void sig(int sign_)
{
    for(int i = 0 ; i < THREAD_NUM ; i++)
    {
        if(!schedulers[i]->IsStop()) schedulers[i]->Stop();
    }
}

int main()
{
    signal(SIGINT,sig);
    spdlog::flush_on(spdlog::level::info);
    spdlog::init_thread_pool(8192, 1);
    auto rotating_sink = ::std::make_shared<spdlog::sinks::rotating_file_sink_mt>("client.log", 1024*1024*10, 3);
    ::std::vector<spdlog::sink_ptr> sinks {rotating_sink};
    auto logger = ::std::make_shared<spdlog::async_logger>("loggername", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
    spdlog::set_default_logger(logger);
    auto test = []()
    {
        auto scheduler = Scheduler_Factory::create_epoll_scheduler(1024, 5);
        schedulers.push_back(scheduler);
        // auto threadpool = Pool_Factory::create_threadpool(4);
        GY_TcpCoroutine<> t = func(scheduler);
        scheduler->Start();
    };
    ::std::vector<::std::thread> ths;
    uint64_t start = Timer::GetCurrentTime();
    for(int i = 0 ; i < THREAD_NUM ; i ++){
        ths.push_back(::std::thread(test));
    }
    for(int i = 0;i < THREAD_NUM ; i++){
        ths[i].join();
    }
    uint64_t end = Timer::GetCurrentTime();
    ::std::cout<<"spend : " << (end - start) << " ms\n";
    return 0;
}
