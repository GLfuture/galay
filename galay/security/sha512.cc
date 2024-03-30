#include "sha512.h"


std::string
galay::Security::Sha512Util::encode(const std::string& str)
{
    unsigned char res[SHA512_DIGEST_LENGTH] = {0};
    SHA512(reinterpret_cast<const unsigned char*>(str.c_str()),str.length(),res);
    return std::string(reinterpret_cast<const char*>(res),SHA512_DIGEST_LENGTH);
}

