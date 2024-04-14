#ifndef GALAY_RANDOM_H
#define GALAY_RANDOM_H

namespace galay
{
    namespace Helper
    {
        class Random
        {
        public:
            static int random(int RandomMin, int RandomMax);

            static double random(double RandomMin, double RandomMax);
        };
    }
}

#endif