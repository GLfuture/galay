#include "Md5.h"

namespace galay::algorithm
{
    std::string 
    Md5Util::Encode(std::string const& str)
    {
        unsigned char digest[MD5_DIGEST_LENGTH];
        memset(digest,0,MD5_DIGEST_LENGTH);
        MD5_CTX ctx;
        MD5_Init(&ctx);
        MD5_Update(&ctx,str.c_str(),str.length());
        MD5_Final(digest,&ctx);
        std::stringstream ss;
        for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
        {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
        }
        return ss.str();
    }

    #if __cplusplus >= 201703L
    std::string 
    Md5Util::Encode(std::string_view str)
    {
        unsigned char digest[MD5_DIGEST_LENGTH];
        memset(digest,0,MD5_DIGEST_LENGTH);
        MD5_CTX ctx;
        MD5_Init(&ctx);
        MD5_Update(&ctx,str.cbegin(),str.length());
        MD5_Final(digest,&ctx);
        std::stringstream ss;
        for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
        {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
        }
        return ss.str();
    }
    #endif
}