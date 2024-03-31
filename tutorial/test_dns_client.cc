#include "../galay/factory/factory.h"
#include <signal.h>
using namespace galay;

std::vector<std::string> hostname = {"www.baidu.com","www.sogou.com","www.qq.com","www.163.com"};

Task<> func(Scheduler_Base::wptr scheduler)
{
    auto client = Client_Factory::create_dns_client(scheduler);
    char buffer[1024] = {0};
    IOFuntion::Addr addr;
    protocol::Dns_Request::ptr request = std::make_shared<protocol::Dns_Request>();
    protocol::Dns_Response::ptr response = std::make_shared<protocol::Dns_Response>();
    auto& header = request->get_header();
    header.m_flags.m_rd = 1;
    header.m_id = 100;
    header.m_questions = 1;
    galay::protocol::Dns_Question question;
    question.m_class = 1;
    question.m_type = galay::protocol::DNS_QUERY_A;
    for(int i = 0 ; i < hostname.size() ; i ++)
    {
        question.m_qname = hostname[i];
        request->get_question_queue().push(question);
        auto ret = co_await client->request(request, response, "114.114.114.114", 53);
    }
    auto& q = response->get_answer_queue();
    while(!q.empty())
    {
        galay::protocol::Dns_Answer answer = q.front();
        q.pop();
        if(answer.m_type == protocol::DNS_QUERY_CNAME){
            std::cout << answer.m_aname << " has cname : " << answer.m_data << '\n';
        }else if(answer.m_type == protocol::DNS_QUERY_A){
            std::cout << answer.m_aname << " has ipv4 : "<< answer.m_data <<'\n';
        }
    }
    scheduler.lock()->stop();
    
}

void sig_handle(int sig)
{

}


int main()
{
    signal(SIGINT,sig_handle);
    auto scheduler = Scheduler_Factory::create_epoll_scheduler(1024,5);
    //一定要用变量接收协程帧，不然会销毁
    Task<> t = func(scheduler);
    scheduler->start();

    return 0;
}