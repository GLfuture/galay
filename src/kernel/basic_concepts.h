#ifndef GALAY_CONCEPTS_H
#define GALAY_CONCEPTS_H
#include <string>
#include <concepts>

namespace galay
{
    template <typename T>
    concept Request = requires(T a, const std::string& buffer,int& state) {
        {
            a.decode(buffer,state)
        } -> std::same_as<int>;
    };
    template <typename T>
    concept Response = requires(T a) {
        {
            a.encode()
        } -> std::same_as<std::string>;
    };

}
#endif