#include "../galay/galay.h"
#include <iostream>

galay::thread::ThreadPool::ptr tp;
std::atomic_int count = 4;

void print(int i)
{
    std::cout << i << std::endl;
}

void test()
{
    for(int i = 0 ; i < 100; i++)
    {
        print(i);
    }
    count.fetch_sub(1);
    if(count.load() == 0){
        tp->Stop();
    }
}

int main()
{
    tp = std::make_shared<galay::thread::ThreadPool>();
    tp->Start(4);
    for(int i = 0 ; i < 4 ; i ++){
        tp->AddTask(test);
    }
    tp->WaitForAllDone();
    return 0;
}