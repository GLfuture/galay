#include "random.h"
#include <random>

int 
galay::util::Random::random(int RandomMin,int RandomMax)
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<> dis(RandomMin,RandomMax);
    return dis(gen);
}


double 
galay::util::Random::random(double RandomMin, double RandomMax)
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_real_distribution<> dis(RandomMin,RandomMax);
    return dis(gen);
}