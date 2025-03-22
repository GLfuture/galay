#include "galay/utils/CricuitBreaker.hpp"

void test(galay::utils::CircuitBreaker& breaker)
{
    if(breaker.allowRequest()) {
        breaker.onSuccess();
    }
}

int main() {

    galay::utils::CircuitBreaker::Config config;
    config.failure_threshold = 20;
    config.reset_timeout = std::chrono::seconds(5);
    config.slow_threshold = 10;
    config.success_threshold = 20;
    galay::utils::CircuitBreaker breaker(config);
    
    return 0;
}