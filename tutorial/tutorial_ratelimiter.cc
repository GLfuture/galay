#include "galay/galay.h"
#include <iostream>
#include <map>
#include <sstream>
#include <iomanip>

#define LIMITE_RATE         50 * 1024 * 1024
#define CAPACITY            10 * 1024 * 1024

// 网络发送字节数，用于统计
unsigned long long sendCount = 0;
std::mutex mutexCount;
std::map<unsigned int, unsigned long long> mapTheadIdCount;

// 网络数据发送测试线程函数
void sendDatatoNet(galay::tools::RateLimiter* speedLimiter)
{
    // 每次发送的数据包大小
    const int sizeOnePacket = 2 * 1024;
    while(true)
    {
        // 获取令牌
        if (!speedLimiter->Pass(sizeOnePacket))
        {
            continue;
        }
        // 统计总的发送包数量
        std::unique_lock<std::mutex> lck(mutexCount);
        sendCount += sizeOnePacket;
        // 统计每个线程发送的数包
        auto threadId = std::this_thread::get_id();
        auto theId = *(unsigned int *)&threadId;
        auto it = mapTheadIdCount.find(theId);
        if (it != mapTheadIdCount.end())
        {
            it->second += sizeOnePacket;
        }
        else
        {
            mapTheadIdCount.insert(std::make_pair(theId, sizeOnePacket));
        }
    }
}

void statisticNetwork()
{
    auto lastTime = std::chrono::steady_clock::now();
    int i = 0;
    while(i++ < 20)
    {
        // 1秒统计一次
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        // 计算投递时间差
        auto curTime = std::chrono::steady_clock::now();
        auto elapsedMs = std::chrono::duration<double, std::milli>(curTime - lastTime).count();

        lastTime = curTime;

        // 打印总速率
        std::unique_lock<std::mutex> lck(mutexCount);
        if (elapsedMs > 0)
        {
            // * 1000 / elapsedMs为毫秒转换为秒
            auto curSpeed = (double)sendCount * 1000 /  1024 / 1024 / elapsedMs;
            std::cout << "speed: " << curSpeed << " MB/s" << std::endl;
        }

        // 打印每个线程发送的百分比
        std::cout << "thread send count: ";
        for (auto it: mapTheadIdCount)
        {
            std::cout << it.first << "(" << std::setfill(' ') << std::setw(2) << 100 * it.second / sendCount  << "%),";
        }
        std::cout << std::endl;
        mapTheadIdCount.clear();
        sendCount = 0;
    }
}

int main(int argc, char *argv[])
{
    // 构造限速器：限速50M/s,容量为6M,间隔10ms投递令牌；当前的流量峰值为60M
    galay::tools::RateLimiter speedLimiter(LIMITE_RATE,CAPACITY, 10);
    speedLimiter.Start();
    // 启动模拟网络发送线程
    for (int i = 0; i < 10; ++i)
    {
        new std::thread(sendDatatoNet, &speedLimiter);
    }
    // 启动统计
    statisticNetwork();
    speedLimiter.Stop();
    return 0;
}