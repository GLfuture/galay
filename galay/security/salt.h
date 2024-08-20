#ifndef GALAY_SALT_H
#define GALAY_SALT_H

#include <string>

namespace galay::security
{
    class Salt{
    public:
        //创建SaltLenMin - SaltLenMax长度的盐值
        static std::string Create(int SaltLenMin,int SaltLenMax);

    };
    
}


#endif