#ifndef GY_SALT_H
#define GY_SALT_H

#include <string>

namespace galay
{
    namespace Security{
        class Salt{
        public:
            //创建SaltLenMin - SaltLenMax长度的盐值
            static std::string create(int SaltLenMin,int SaltLenMax);

        };
    }
}


#endif