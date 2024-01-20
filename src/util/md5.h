#ifndef GALAY_MD5_H
#define GALAY_MD5_H


#include <openssl/md5.h>
#include <string>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <iostream>
#if __cplusplus >= 201703L
#include <string_view>
#endif

namespace galay{
    class Md5Util
    {
    public:

        static std::string md5_encode(std::string const &str);
        #if __cplusplus >= 201703L
        static std::string md5_encode(std::string_view str);
        #endif
    };

}


#endif