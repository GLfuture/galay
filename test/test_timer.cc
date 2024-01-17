#include "../src/factory/factory.h"
#include "../src/util/md5.h"
#include "../src/util/sha256.h"
#include <fstream>
#include <signal.h>

using namespace galay;

void sig_handle(int sig)
{

}


int main()
{
    signal(SIGINT,sig_handle);
    auto scheduler = Scheduler_Factory::create_scheduler(IO_EPOLL, 1024, -1);
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