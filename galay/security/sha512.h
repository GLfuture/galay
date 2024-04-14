#ifndef GALAY_SHA512_H
#define GALAY_SHA512_H

#include <string>
#include <openssl/sha.h>

namespace galay{
    namespace Security{
        class Sha512Util{
        public:
            static std::string encode(const std::string& str);
        };
    }
}



#endif