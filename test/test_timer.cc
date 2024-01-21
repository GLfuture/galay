#include "../galay/factory/factory.h"
#include "../galay/util/md5.h"
#include "../galay/util/sha256.h"
#include <fstream>
#include <signal.h>

using namespace galay;

Scheduler_Base::ptr scheduler;

void sig_handle(int sig)
{
    scheduler->stop();
}


int main()
{
    signal(SIGINT,sig_handle);
    scheduler = Scheduler_Factory::create_select_scheduler(0);
    scheduler->get_timer_manager()->add_timer(Timer_Factory::create_timer(500, 1, []() {
        std::ifstream in("/home/gong/projects/galay/1.txt");
        if(in.fail()){
            std::cout<<"fail\n";
        }
        char buffer[1024] = {0};
        in.read(buffer,1024);
        int byte = in.gcount();
        std::string str(buffer,byte);
        std::cout<<Md5Util::md5_encode(str)<<'\n';
        std::cout << Sha256Util::sha256_encode(str) << '\n';

    }));

    scheduler->start();
    return 0;
}