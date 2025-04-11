#include "Random.h"
#include <random>

namespace galay::utils
{
int 
Randomizer::RandomInt(int RandomMin,int RandomMax)
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<int> dis(RandomMin,RandomMax);
    return dis(gen);
}

uint32_t 
Randomizer::RandomUint32(uint32_t RandomMin, uint32_t RandomMax)
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint32_t> dis(RandomMin,RandomMax);
    return dis(gen);
}

uint64_t 
Randomizer::RandomUint64(uint64_t RandomMin, uint64_t RandomMax)
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis(RandomMin,RandomMax);
    return dis(gen);
}

double 
Randomizer::RandomDouble(double RandomMin, double RandomMax)
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_real_distribution<double> dis(RandomMin,RandomMax);
    return dis(gen);
}
}