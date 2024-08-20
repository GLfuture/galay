#ifndef GALAY_RANDOM_H
#define GALAY_RANDOM_H

#include <cstdint>

namespace galay::tools
{
    class Randomizer
    {
    public:
        static int RandomInt(int RandomMin, int RandomMax);
        static uint32_t RandomUint32(uint32_t RandomMin, uint32_t RandomMax);
        static uint64_t RandomUint64(uint64_t RandomMin, uint64_t RandomMax);
        static double RandomDouble(double RandomMin, double RandomMax);
    };
}

#endif