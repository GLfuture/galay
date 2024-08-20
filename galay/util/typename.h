#ifndef GALAY_TYPENAME_H
#define GALAY_TYPENAME_H

#include <string>
#include <cxxabi.h>

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