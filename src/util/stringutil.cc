#include "stringutil.h"

std::vector<std::string> galay::StringUtil::Spilt_With_Char(const std::string &str, const char symbol)
{
    std::vector<std::string> result;
    for (int i = 0; i < str.size(); i++)
    {
        int beg = i;
        i = str.find(symbol, beg);
        result.emplace_back(str.substr(beg, i - beg));
        if (i == std::string::npos)
            break;
    }
    return result;
}


std::vector<std::string> galay::StringUtil::Spilt_With_Str(const std::string &str, const std::string& symbol)
{
    std::vector<std::string> result;
    if (symbol.empty())
        return result;
    size_t len = symbol.length();
    for (int i = 0; i < str.size(); i++)
    {
        int beg = i;
        i = str.find(symbol, beg);
        result.emplace_back(str.substr(beg, i - beg));
        if (i == std::string::npos)
            break;
        i += len - 1;
    }
    return result;
}


std::vector<std::string> galay::StringUtil::Spilt_With_Char_Connect_With_char(const std::string &str, const char partition, const char connction)
{
    int beg = 0, end = 0;
    uint16_t status = 0;
    std::vector<std::string> result;
    for (int i = 0; i < str.size(); i++)
    {
        if (str[i] == '\\')
        {
            i++;
            end++;
            if (i > str.size())
            {
                result.clear();
                return result;
            }
            continue;
        }
        else if (str[i] == connction)
        {
            if (status == 0)
            {
                beg = i + 1;
                end++;
                status = 1;
                continue;
            }
            else
            {
                if (beg != end)
                    result.emplace_back(str.substr(beg, end - beg));
                end++;
                beg = end;
                status = 0;
                continue;
            }
        }
        else if (str[i] == partition && status == 0)
        {
            if (beg != end)
                result.emplace_back(str.substr(beg, end - beg));
            beg = end + 1;
        }
        end++;
        if (i == str.size() - 1)
        {
            if (beg != end)
                result.emplace_back(str.substr(beg, end - beg));
        }
    }
    if (status)
    {
        result.clear();
        return result;
    }
    return result;
}


#if __cplusplus >= 201703L
std::vector<std::string_view> galay::StringUtil::Spilt_With_Char(std::string_view str, const char symbol)
{
    std::vector<std::string_view> result;
    for (int i = 0; i < str.size(); i++)
    {
        int beg = i;
        i = str.find(symbol, beg);
        result.emplace_back(str.substr(beg, i - beg));
        if (i == std::string::npos)
            break;
    }
    return result;
}

std::vector<std::string_view> galay::StringUtil::Spilt_With_Str(std::string_view str, std::string_view symbol)
{
    std::vector<std::string_view> result;
    if (symbol.empty())
        return result;
    size_t len = symbol.length();
    for (int i = 0; i < str.size(); i++)
    {
        int beg = i;
        i = str.find(symbol, beg);
        result.emplace_back(str.substr(beg, i - beg));
        if (i == std::string::npos)
            break;
        i += len - 1;
    }
    return result;
}

std::vector<std::string_view> galay::StringUtil::Spilt_With_Char_Connect_With_char(std::string_view str, const char partition, const char connction)
{
    int beg = 0, end = 0;
    uint16_t status = 0;
    std::vector<std::string_view> result;
    for (int i = 0; i < str.size(); i++)
    {
        if (str[i] == '\\')
        {
            i++;
            end++;
            if (i > str.size())
            {
                result.clear();
                return result;
            }
            continue;
        }
        else if (str[i] == connction)
        {
            if (status == 0)
            {
                beg = i + 1;
                end++;
                status = 1;
                continue;
            }
            else
            {
                if (beg != end)
                    result.emplace_back(str.substr(beg, end - beg));
                end++;
                beg = end;
                status = 0;
                continue;
            }
        }
        else if (str[i] == partition && status == 0)
        {
            if (beg != end)
                result.emplace_back(str.substr(beg, end - beg));
            beg = end + 1;
        }
        end++;
        if (i == str.size() - 1)
        {
            if (beg != end)
                result.emplace_back(str.substr(beg, end - beg));
        }
    }
    if (status)
    {
        result.clear();
        return result;
    }
    return result;
}

#endif