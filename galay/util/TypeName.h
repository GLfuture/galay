#ifndef __GALAY_TYPENAME_H__
#define __GALAY_TYPENAME_H__

#include <string>
#if  defined(WIN32) || defined(_WIN32) || defined(_WIN32_) || defined(WIN64) || defined(_WIN64) || defined(_WIN64_)
#elif
#include <cxxabi.h>
#endif


namespace galay::util
{
    template <typename T>
    std::string GetTypeName()
    {
        std::string typeName;
        char *szDemangleName = nullptr;
#ifdef __GNUC__
        szDemangleName = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
#else
        szDemangleName = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
#endif
        if (nullptr != szDemangleName)
        {
            typeName = szDemangleName;
            free(szDemangleName);
            return typeName;
        }
        return "";
    }
}

#endif