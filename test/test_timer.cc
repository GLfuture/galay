#include "../src/factory/factory.h"

using namespace galay;

int main()
{
    auto scheduler = Scheduler_Factory::create_scheduler(IO_EPOLL,1024,-1);

    scheduler->get_timer_manager()->add_timer(Timer_Factory::create_timer(5000, 5, []()
                                                                          { std::cout << "调用\n"; }));
    

    scheduler->start();
    return 0;
}