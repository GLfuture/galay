#include "Sha256.h"

namespace galay::algorithm
{
std::string 
Sha256Util::Encode(const std::string & str)
{
    EVP_MD_CTX* md_ctx = EVP_MD_CTX_new();
    EVP_DigestInit(md_ctx, EVP_sha256());
    EVP_DigestUpdate(md_ctx, str.c_str(), str.length());

    unsigned char hash[SHA256_DIGEST_LENGTH];
    unsigned int hash_len;
    EVP_DigestFinal(md_ctx, hash, &hash_len);

    EVP_MD_CTX_free(md_ctx);

    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < hash_len; i++) {
        ss << std::setw(2) << static_cast<unsigned int>(hash[i]);
    }
    return ss.str();
}


std::string 
Sha256Util::Encode(std::string_view str)
{
    EVP_MD_CTX* md_ctx = EVP_MD_CTX_new();
    EVP_DigestInit(md_ctx, EVP_sha256());
    EVP_DigestUpdate(md_ctx, str.cbegin(), str.length());

    unsigned char hash[SHA256_DIGEST_LENGTH];
    unsigned int hash_len;
    EVP_DigestFinal(md_ctx, hash, &hash_len);

    EVP_MD_CTX_free(md_ctx);

    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < hash_len; i++) {
        ss << std::setw(2) << static_cast<unsigned int>(hash[i]);
    }
    return ss.str();
}


}