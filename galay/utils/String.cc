#include "String.h"

namespace galay::utils
{
std::vector<std::string> 
StringSplitter::SpiltWithChar(const std::string &str, const char symbol)
{
    std::vector<std::string> result;
    for (int i = 0; i < str.size(); i++)
    {
        int beg = i;
        i = str.find(symbol, beg);
        if (i == std::string::npos) {
            result.emplace_back(str.substr(beg));
            break;
        }
        std::string temp = str.substr(beg, i - beg);
        if(!temp.empty()) result.emplace_back(std::move(temp));
    }
    return result;
}


std::vector<std::string>
StringSplitter::SpiltWithStr(const std::string &str, const std::string& symbol)
{
    std::vector<std::string> result;
    if (symbol.empty())
        return result;
    size_t len = symbol.length();
    for (int i = 0; i < str.size(); i++)
    {
        int beg = i;
        i = str.find(symbol, beg);
        std::string temp = str.substr(beg, i - beg);
        if(!temp.empty()) result.emplace_back(std::move(temp));
        if (i == std::string::npos)
            break;
        i += len - 1;
    }
    return result;
}


std::vector<std::string> 
StringSplitter::SpiltWithCharAndConnectWithchar(const std::string &str, const char partition, const char connction)
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
                {
                    std::string temp = str.substr(beg, end - beg);
                    if(!temp.empty()) result.emplace_back();
                }
                end++;
                beg = end;
                status = 0;
                continue;
            }
        }
        else if (str[i] == partition && status == 0)
        {
            if (beg != end)
            {
                std::string temp = str.substr(beg, end - beg);
                if(!temp.empty()) result.emplace_back();
            }
            beg = end + 1;
        }
        end++;
        if (i == str.size() - 1)
        {
            if (beg != end)
            {
                std::string temp = str.substr(beg, end - beg);
                if(!temp.empty()) result.emplace_back();
            }
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
std::vector<std::string_view> 
StringSplitter::SpiltWithChar(std::string_view str, const char symbol)
{
    std::vector<std::string_view> result;
    for (int i = 0; i < str.size(); i++)
    {
        int beg = i;
        i = str.find(symbol, beg);
        if (i == std::string::npos) {
            result.emplace_back(str.substr(beg));
            break;
        }
        std::string_view temp = str.substr(beg, i - beg);
        if(!temp.empty()) result.emplace_back(std::move(temp));
    }
    return result;
}

std::vector<std::string_view> 
StringSplitter::SpiltWithStr(std::string_view str, std::string_view symbol)
{
    std::vector<std::string_view> result;
    if (symbol.empty())
        return result;
    size_t len = symbol.length();
    for (int i = 0; i < str.size(); i++)
    {
        int beg = i;
        i = str.find(symbol, beg);
        std::string_view temp = str.substr(beg, i - beg);
        if(!temp.empty()) result.emplace_back(temp);
        if (i == std::string::npos)
            break;
        i += len - 1;
    }
    return result;
}

std::vector<std::string_view> 
StringSplitter::SpiltWithCharAndConnectWithchar(std::string_view str, const char partition, const char connction)
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
                {
                    std::string_view temp = str.substr(beg, end - beg);
                    if(!temp.empty()) result.emplace_back();;
                }
                end++;
                beg = end;
                status = 0;
                continue;
            }
        }
        else if (str[i] == partition && status == 0)
        {
            if (beg != end)
            {
                std::string_view temp = str.substr(beg, end - beg);
                if(!temp.empty()) result.emplace_back();;
            }
            beg = end + 1;
        }
        end++;
        if (i == str.size() - 1)
        {
            if (beg != end)
            {
                std::string_view temp = str.substr(beg, end - beg);
                if(!temp.empty()) result.emplace_back();;
            }
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


}