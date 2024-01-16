#include "../src/factory/factory.h"

using namespace galay;


std::mutex mtx;

int print(int limit)
{
    std::unique_lock lock(mtx);
    for(int i = 0 ; i < limit ; i ++)
    {
        std::cout<< i <<" ";
    }
    std::cout<<"finish\n";
    return limit;
}


int main()
{
    auto pool = Pool_Factory::create_threadpool(10);
    auto fu1 = pool->exec(print,10);
    auto fu2 = pool->exec(print,20);
    auto fu3 = pool->exec(print,10);
    std::this_thread::sleep_for(std::chrono::seconds(10));
    std::cout << "fu1:" << fu1.get() << '\n';
    std::cout << "fu2:" << fu2.get() << '\n';
    std::cout << "fu3:" << fu3.get() << '\n';
    return 0;
}