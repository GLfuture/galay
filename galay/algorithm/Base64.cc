#include "Base64.h"

namespace galay::algorithm 
{
   //
 // Depending on the url parameter in base64_chars, one of
 // two sets of base64 characters needs to be chosen.
 // They differ in their last two characters.
 //



std::string 
Base64Util::Base64Encode(unsigned char const* bytes_to_encode, size_t in_len, bool url) {

    size_t len_encoded = (in_len +2) / 3 * 4;

    unsigned char trailing_char = url ? '.' : '=';

 //
 // Choose set of base64 characters. They differ
 // for the last two positions, depending on the url
 // parameter.
 // A bool (as is the parameter url) is guaranteed
 // to evaluate to either 0 or 1 in C++ therefore,
 // the correct character set is chosen by subscripting
 // base64_chars with url.
 //
    const char* base64_chars_ = base64_chars[url];

    std::string ret;
    ret.reserve(len_encoded);

    unsigned int pos = 0;

    while (pos < in_len) {
        ret.push_back(base64_chars_[(bytes_to_encode[pos + 0] & 0xfc) >> 2]);

        if (pos+1 < in_len) {
           ret.push_back(base64_chars_[((bytes_to_encode[pos + 0] & 0x03) << 4) + ((bytes_to_encode[pos + 1] & 0xf0) >> 4)]);

           if (pos+2 < in_len) {
              ret.push_back(base64_chars_[((bytes_to_encode[pos + 1] & 0x0f) << 2) + ((bytes_to_encode[pos + 2] & 0xc0) >> 6)]);
              ret.push_back(base64_chars_[  bytes_to_encode[pos + 2] & 0x3f]);
           }
           else {
              ret.push_back(base64_chars_[(bytes_to_encode[pos + 1] & 0x0f) << 2]);
              ret.push_back(trailing_char);
           }
        }
        else {

            ret.push_back(base64_chars_[(bytes_to_encode[pos + 0] & 0x03) << 4]);
            ret.push_back(trailing_char);
            ret.push_back(trailing_char);
        }

        pos += 3;
    }


    return ret;
}


std::string 
Base64Util::Base64Decode(std::string const& s, bool remove_linebreaks) {
   return Decode(s, remove_linebreaks);
}

std::string 
Base64Util::Base64Encode(std::string const& s, bool url) {
   return Encode(s, url);
}

std::string 
Base64Util::Base64EncodePem (std::string const& s) {
   return encode_pem(s);
}

std::string 
Base64Util::Base64EncodeMime(std::string const& s) {
   return encode_mime(s);
}

#if __cplusplus >= 201703L
//
// Interface with std::string_view rather than const std::string&
// Requires C++17
// Provided by Yannic Bonenberger (https://github.com/Yannic)
//

std::string 
Base64Util::Base64Encode(std::string_view s, bool url) {
   return Encode(s, url);
}

std::string 
Base64Util::Base64EncodePem(std::string_view s) {
   return encode_pem(s);
}

std::string 
Base64Util::Base64EncodeMime(std::string_view s) {
   return encode_mime(s);
}

std::string 
Base64Util::Base64Decode(std::string_view s, bool remove_linebreaks) {
   return Decode(s, remove_linebreaks);
}

#endif  // __cplusplus >= 201703L

}