#ifndef GY_RANDOM_H
#define GY_RANDOM_H


namespace galay{
    class Random{
    public:
        static int random(int RandomMin,int RandomMax);

        static double random(double RandomMin, double RandomMax);
    };
}



#endif