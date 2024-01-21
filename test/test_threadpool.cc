#include "../galay/factory/factory.h"

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
    if(fu1.valid())std::cout << "fu1:" << fu1.get() << '\n';
    if(fu2.valid()) std::cout << "fu2:" << fu2.get() << '\n';
    if(fu3.valid()) std::cout << "fu3:" << fu3.get() << '\n';

    pool->destroy();
    return 0;
}