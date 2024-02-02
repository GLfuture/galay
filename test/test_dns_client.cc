#include "../galay/factory/factory.h"

using namespace galay;

Task<> func(Scheduler_Base::wptr scheduler)
{
    auto client = Client_Factory::create_dns_client(scheduler);
    char buffer[1024] = {0};
    iofunction::Addr addr;
    protocol::Dns_Request::ptr request = std::make_shared<protocol::Dns_Request>();
    protocol::Dns_Response::ptr response = std::make_shared<protocol::Dns_Response>();
    auto& header = request->get_header();
    header.m_flags.m_rd = 1;
    header.m_id = 100;
    header.m_questions = 1;
    galay::protocol::Dns_Question question;
    question.m_qname = "www.baidu.com";
    question.m_class = 1;
    question.m_type = galay::protocol::DNS_QUERY_A;
    request->get_question_queue().push(question);
    int ret = co_await client->request(request,response,"114.114.114.114",53);
    std::cout<<ret<<'\n';
    auto& q = response->get_answer_queue();
    while(!q.empty())
    {
        galay::protocol::Dns_Answer answer = q.front();
        q.pop();
        std::cout<<answer.m_data<<'\n';
    }
    
}

int main()
{
    auto scheduler = Scheduler_Factory::create_epoll_scheduler(1024,5);
    //一定要用变量接收协程帧，不然会销毁
    Task<> t = func(scheduler);
    scheduler->start();

    return 0;
}