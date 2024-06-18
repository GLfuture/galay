#include "../galay/galay.h"
#include <iostream>
#include <spdlog/spdlog.h>

galay::DnsAsyncClient client("114.114.114.114");
galay::GY_Coroutine<galay::CoroutineStatus> func()
{
    std::queue<std::string> q;
    q.push("www.baidu.com");
    q.push("www.sogou.com");
    q.push("qq.com");
    auto resp = co_await client.QueryA(q);
    auto ans = resp->GetAnswerQueue();
    while(!ans.empty())
    {
        auto item = ans.front();
        ans.pop();
        if(item.m_type == galay::protocol::dns::kDnsQueryCname){
            std::cout << item.m_aname << " has cname : " << item.m_data << '\n';
        }else if(item.m_type == galay::protocol::dns::kDnsQueryA){
            std::cout << item.m_aname << " has ipv4 : "<< item.m_data <<'\n';
        }
    }
    resp = co_await client.QueryAAAA(q);
    ans = resp->GetAnswerQueue();
    while(!ans.empty())
    {
        auto item = ans.front();
        ans.pop();
        if(item.m_type == galay::protocol::dns::kDnsQueryCname){
            std::cout << item.m_aname << " has cname : " << item.m_data << '\n';
        }else if(item.m_type == galay::protocol::dns::kDnsQueryAAAA){
            std::cout << item.m_aname << " has ipv6 : "<< item.m_data <<'\n';
        }
    }
    client.Close();
    co_return galay::CoroutineStatus::kCoroutineFinished;
}

int main()
{
    //遇到debug级别日志立即刷新
    //spdlog::set_level(spdlog::level::err);
    auto co = func();
    getchar();
    return 0;
}
