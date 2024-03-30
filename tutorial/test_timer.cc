#include "../galay/factory/factory.h"
#include "../galay/security/md5.h"
#include "../galay/security/sha256.h"
#include "../galay/security/sha512.h"
#include "../galay/security/salt.h"
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
    auto timer = scheduler->get_timer_manager()->add_timer(100, 1, []() ->void {
        std::string salt = Security::Salt::create(10,20);
        std::string password = "123456";
        // std::cout << "origin: " <<  str << '\n';
        // std::cout << "md5 :" << Security::Md5Util::encode(str)<<'\n';
        // std::cout << "sha256 :" << Security::Sha256Util::encode(str) << '\n';
        // std::cout << "sha512 :" << Security::Sha512Util::encode(str)<<'\n';
        std::cout << salt << '\n';
        std::cout << Security::Sha256Util::encode(Security::Md5Util::encode(password) + salt);

    });
    scheduler->get_timer_manager()->add_timer(100,1,[timer](){
        timer->cancle();
        std::cout<<"cancle\n";
    });
    scheduler->start();
    return 0;
}