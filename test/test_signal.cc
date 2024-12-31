#include "galay/galay.hpp"
#include <iostream>
#include <csignal>

int main()
{
    galay::common::SignalFactory::GetInstance()->SetSignalHandler(SIGINT, [](int sig) {
        std::cout << "SIGINT1" << std::endl;
    });

    galay::common::SignalFactory::GetInstance()->SetSignalHandler(SIGINT, [](int sig) {
        std::cout << "SIGINT2" << std::endl;
    });

    galay::common::SignalFactory::GetInstance()->SetSignalHandler(SIGABRT, [](int sig) {
        std::cout << "SIGABRT" << std::endl;
    });
    getchar();
    abort();
    return 0;
}