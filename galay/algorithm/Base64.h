
#ifndef __GALAY_BASE64_H__
#define __GALAY_BASE64_H__

#include <string>
#include <algorithm>
#include <stdexcept>

#if __cplusplus >= 201703L
#include <string_view>
#endif

namespace galay::algorithm
{
    static const char *base64_chars[2] = {
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789"
        "+/",

        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789"
        "-_"};

    class Base64Util
    {
    public:
        static std::string Base64Encode(std::string const &s, bool url = false);
        static std::string Base64EncodePem(std::string const &s);
        static std::string Base64EncodeMime(std::string const &s);

        static std::string Base64Decode(std::string const &s, bool remove_linebreaks = false);
        static std::string Base64Encode(unsigned char const *, size_t len, bool url = false);

#if __cplusplus >= 201703L

        static std::string Base64Encode(std::string_view s, bool url = false);
        static std::string Base64EncodePem(std::string_view s);
        static std::string Base64EncodeMime(std::string_view s);

        static std::string Base64Decode(std::string_view s, bool remove_linebreaks = false);
#endif
    private:
        template <typename String>
        static std::string Decode(String const &encoded_string, bool remove_linebreaks)
        {
            //
            // Decode(…) is templated so that it can be used with String = const std::string&
            // or std::string_view (requires at least C++17)
            //

            if (encoded_string.empty())
                return std::string();

            if (remove_linebreaks)
            {

                std::string copy(encoded_string);

                copy.erase(std::remove(copy.begin(), copy.end(), '\n'), copy.end());

                return Base64Decode(copy, false);
            }

            size_t length_of_string = encoded_string.length();
            size_t pos = 0;

            //
            // The approximate length (bytes) of the decoded string might be one or
            // two bytes smaller, depending on the amount of trailing equal signs
            // in the encoded string. This approximation is needed to reserve
            // enough space in the string to be returned.
            //
            size_t approx_length_of_decoded_string = length_of_string / 4 * 3;
            std::string ret;
            ret.reserve(approx_length_of_decoded_string);

            while (pos < length_of_string)
            {
                //
                // Iterate over encoded input string in chunks. The size of all
                // chunks except the last one is 4 bytes.
                //
                // The last chunk might be padded with equal signs or dots
                // in order to make it 4 bytes in size as well, but this
                // is not required as per RFC 2045.
                //
                // All chunks except the last one produce three output bytes.
                //
                // The last chunk produces at least one and up to three bytes.
                //

                size_t pos_of_char_1 = pos_of_char(encoded_string.at(pos + 1));

                //
                // Emit the first output byte that is produced in each chunk:
                //
                ret.push_back(static_cast<std::string::value_type>(((pos_of_char(encoded_string.at(pos + 0))) << 2) + ((pos_of_char_1 & 0x30) >> 4)));

                if ((pos + 2 < length_of_string) && // Check for data that is not padded with equal signs (which is allowed by RFC 2045)
                    encoded_string.at(pos + 2) != '=' &&
                    encoded_string.at(pos + 2) != '.' // accept URL-safe base 64 strings, too, so check for '.' also.
                )
                {
                    //
                    // Emit a chunk's second byte (which might not be produced in the last chunk).
                    //
                    unsigned int pos_of_char_2 = pos_of_char(encoded_string.at(pos + 2));
                    ret.push_back(static_cast<std::string::value_type>(((pos_of_char_1 & 0x0f) << 4) + ((pos_of_char_2 & 0x3c) >> 2)));

                    if ((pos + 3 < length_of_string) &&
                        encoded_string.at(pos + 3) != '=' &&
                        encoded_string.at(pos + 3) != '.')
                    {
                        //
                        // Emit a chunk's third byte (which might not be produced in the last chunk).
                        //
                        ret.push_back(static_cast<std::string::value_type>(((pos_of_char_2 & 0x03) << 6) + pos_of_char(encoded_string.at(pos + 3))));
                    }
                }

                pos += 4;
            }

            return ret;
        }

        static unsigned int pos_of_char(const unsigned char chr)
        {
            //
            // Return the position of chr within Base64Encode()
            //

            if (chr >= 'A' && chr <= 'Z')
                return chr - 'A';
            else if (chr >= 'a' && chr <= 'z')
                return chr - 'a' + ('Z' - 'A') + 1;
            else if (chr >= '0' && chr <= '9')
                return chr - '0' + ('Z' - 'A') + ('z' - 'a') + 2;
            else if (chr == '+' || chr == '-')
                return 62; // Be liberal with input and accept both url ('-') and non-url ('+') base 64 characters (
            else if (chr == '/' || chr == '_')
                return 63; // Ditto for '/' and '_'
            else
                //
                // 2020-10-23: Throw std::exception rather than const char*
                //(Pablo Martin-Gomez, https://github.com/Bouska)
                //
                throw std::runtime_error("Input is not valid base64-encoded data.");
        }

        static std::string insert_linebreaks(std::string str, size_t distance)
        {
            //
            // Provided by https://github.com/JomaCorpFX, adapted by me.
            //
            if (!str.length())
            {
                return "";
            }

            size_t pos = distance;

            while (pos < str.size())
            {
                str.insert(pos, "\n");
                pos += distance + 1;
            }

            return str;
        }

        template <typename String, unsigned int line_length>
        static std::string encode_with_line_breaks(String s)
        {
            return insert_linebreaks(Base64Encode(s, false), line_length);
        }

        template <typename String>
        static std::string encode_pem(String s)
        {
            return encode_with_line_breaks<String, 64>(s);
        }

        template <typename String>
        static std::string encode_mime(String s)
        {
            return encode_with_line_breaks<String, 76>(s);
        }

        template <typename String>
        static std::string Encode(String s, bool url)
        {
            return Base64Encode(reinterpret_cast<const unsigned char *>(s.data()), s.length(), url);
        }
    };

}


#endif /* BASE64_H_C0CE2A47_D10E_42C9_A27C_C883944E704A */