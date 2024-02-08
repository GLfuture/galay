
#include "../galay/factory/factory.h" 

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
        return sizeof(self_head);
    }

    std::string encode()
    {
        char temp[sizeof(self_head)];
        memset(temp,0,sizeof(self_head));
        memcpy(temp,&m_head,sizeof(self_head));
        std::string res(temp,sizeof(self_head));
        res += body;
        return res;
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

    int decode(const std::string &buffer , int &state)
    {
        memcpy(&m_head.version,buffer.substr(0,4).c_str(),sizeof(4));
        memcpy(&m_head.length,buffer.substr(4,sizeof(int)).c_str(),sizeof(int));
        m_head.length = ntohl(m_head.length);
        this->body = buffer.substr(8,m_head.length);
        return sizeof(self_head);
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

Task<> func(Epoll_Scheduler::ptr scheduler)
{
    auto client = std::make_shared<Tcp_Request_Client>(scheduler);
    int ret = co_await client->connect("127.0.0.1",8080);
    if(ret == 0) {
        std::cout<<"connect success\n";
    }else{
        std::cout<<"connect failed\n";
    }
    auto request = std::make_shared<Self_Request>();
    auto response = std::make_shared<Self_Response>();
    self_head& head = request->get_head();
    memcpy(head.version,"1.1",4);
    for(int i = 0 ; i <= 10000 ; i++)
    {
        std::string buffer = std::to_string(i) + ": hello world\n";
        head.length = htonl(buffer.length());
        request->get_body() = buffer;
        ret = co_await client->request(request,response);
        if(i % 1000 == 0) std::cout << i << " " << ret <<"  recv len :" << response->get_head().length << "buffer: " << response->get_body() << '\n';
    }
    sleep(15);
    scheduler->stop();
    co_return;
}


int main()
{
    auto scheduler = Scheduler_Factory::create_epoll_scheduler(1,5);
    //auto threadpool = Pool_Factory::create_threadpool(4);
    Task<> t = func(scheduler);
    scheduler->start();
    std::cout<<"end\n";
    return 0;
}
