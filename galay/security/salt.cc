#include "salt.h"
#include "../util/random.h"
#include <string.h>
#include <openssl/rand.h>

::std::string 
galay::security::Salt::create(int SaltLenMin,int SaltLenMax)
{
    int saltlen = util::Random::random(SaltLenMin,SaltLenMax);
    unsigned char* salt = new unsigned char[saltlen];
    bzero(salt,saltlen);
    RAND_bytes(salt,saltlen);
    ::std::string res(reinterpret_cast<char*>(salt),saltlen);
    delete[] salt;
    return res;
}