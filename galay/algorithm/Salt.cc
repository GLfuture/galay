#include "Salt.h"
#include "galay/utils/Random.h"
#include <string.h>
#include <openssl/rand.h>

namespace galay::algorithm
{
std::string 
Salt::Create(int SaltLenMin,int SaltLenMax)
{
    int saltlen = utils::Randomizer::RandomInt(SaltLenMin,SaltLenMax);
    unsigned char* salt = new unsigned char[saltlen];
    bzero(salt,saltlen);
    RAND_bytes(salt,saltlen);
    std::string res(reinterpret_cast<char*>(salt),saltlen);
    delete[] salt;
    return res;
}
}

