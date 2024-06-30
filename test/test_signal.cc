#include "../galay/galay.h"
#include <iostream>
#include <csignal>

int main()
{
    galay::common::GY_SignalHandler::GetInstance()->SetSignalHandler(SIGINT, [](int sig) {
        std::cout << "SIGINT1" << std::endl;
    });

    galay::common::GY_SignalHandler::GetInstance()->SetSignalHandler(SIGINT, [](int sig) {
        std::cout << "SIGINT2" << std::endl;
    });

    galay::common::GY_SignalHandler::GetInstance()->SetSignalHandler(SIGABRT, [](int sig) {
        std::cout << "SIGABRT" << std::endl;
    });
    getchar();
    abort();
    return 0;
}