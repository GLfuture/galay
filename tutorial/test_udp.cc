#include "galay/galay.hpp"

galay::Coroutine<int> test_dns(galay::RoutineCtx ctx)
{
    galay::AsyncUdpSocket socket(galay::EventSchedulerHolder::GetInstance()->GetScheduler(0));
    if (!socket.Socket())
    {
        printf("create socket failed\n");
        co_return -1;
    }
    printf("create socket success\n");
    galay::dns::DnsHeader header;
    header.m_id = 10;
    header.m_flags.m_rd = 1;
    header.m_questions = 1;
    galay::dns::DnsRequest request;
    request.SetHeader(std::move(header));
    galay::dns::DnsQuestion question;
    question.m_qname = "www.baidu.com";
    question.m_type = galay::dns::kDnsQueryA;
    question.m_class = 1;
    request.SetQuestion(std::move(question));
    galay::IOVecHolder<galay::UdpIOVec> sholder(request.EncodePdu()), rholder(1500);
    sholder->m_addr = galay::THost{
        .m_ip = "8.8.8.8",
        .m_port = 53
    };
    int length = co_await socket.SendTo<int>(&sholder, 1500);
    printf("sendto success %d\n", length);
    length = co_await socket.RecvFrom<int>(&rholder, 1500);
    galay::dns::DnsResponse response;
    size_t size = length;
    response.DecodePdu({rholder->m_buffer, size});
    auto answers = response.GetAnswerQueue();
    while (!answers.empty())
    {
        auto answer = answers.front();
        printf("answer %s %s %s\n", answer.m_aname.c_str(), answer.m_data.c_str(), galay::dns::GetDnsAnswerTypeString(answer.m_type).c_str());
        answers.pop();
    }

    co_return 1;
}

int main()
{
    galay::InitializeGalayEnv({1, -1}, {1, -1}, {1, -1});
    int a = test_dns(galay::RoutineCtx::Create(galay::EventSchedulerHolder::GetInstance()->GetScheduler(0)))().value();
    getchar();
    return 0;
}