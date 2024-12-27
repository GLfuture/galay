#include "galay/galay.h"

galay::Coroutine<int> test_dns()
{
    galay::AsyncUdpSocket socket(galay::EeventSchedulerHolder::GetInstance()->GetScheduler()->GetEngine());
    if (!socket.Socket())
    {
        printf("create socket failed\n");
        co_return -1;
    }
    printf("create socket success\n");
    galay::protocol::dns::DnsHeader header;
    header.m_id = 10;
    header.m_flags.m_rd = 1;
    header.m_questions = 1;
    galay::protocol::dns::DnsRequest request;
    request.SetHeader(std::move(header));
    galay::protocol::dns::DnsQuestion question;
    question.m_qname = "www.baidu.com";
    question.m_type = galay::protocol::dns::kDnsQueryA;
    question.m_class = 1;
    request.SetQuestion(std::move(question));
    galay::IOVecHolder<galay::UdpIOVec> sholder(request.EncodePdu()), rholder(1500);
    sholder->m_addr = galay::NetAddr{
        .m_ip = "8.8.8.8",
        .m_port = 53
    };
    int length = co_await socket.SendTo<int>(&sholder, 1500);
    printf("sendto success %d\n", length);
    length = co_await socket.RecvFrom<int>(&rholder, 1500);
    galay::protocol::dns::DnsResponse response;
    size_t size = length;
    response.DecodePdu({rholder->m_buffer, size});
    auto answers = response.GetAnswerQueue();
    while (!answers.empty())
    {
        auto answer = answers.front();
        printf("answer %s %s %s\n", answer.m_aname.c_str(), answer.m_data.c_str(), galay::protocol::dns::GetDnsAnswerTypeString(answer.m_type).c_str());
        answers.pop();
    }

    co_return 1;
}

int main()
{
    GALAY_APP_MAIN(
        int a = test_dns()().value();
        getchar();
    );
    return 0;
}