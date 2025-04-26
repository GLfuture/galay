#ifndef __GALAY_SALT_H__
#define __GALAY_SALT_H__

#include <string>

namespace galay::algorithm
{
    class Salt{
    public:
        //创建SaltLenMin - SaltLenMax长度的盐值
        static std::string Create(int SaltLenMin,int SaltLenMax);

    };
    
}


#endif