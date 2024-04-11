#ifndef GALAY_CONCEPTS_H
#define GALAY_CONCEPTS_H
#include <string>
#include <concepts>

namespace galay
{
    enum Proto_Judge_Type {
        PROTOCOL_LEGAL,
        PROTOCOL_INCOMPLETE,
        PROTOCOL_ILLEGAL
    };

    template <typename T>
    concept Tcp_Request = requires(T a) {
        { 
            a.DecodePdu(std::declval<const std::string&>()) 
        } -> std::same_as<int>;

        { 
            //会附带解析头部的职责
            a.IsPduAndLegal(std::declval<const std::string&>())
        } -> std::same_as<Proto_Judge_Type>;
    };
    template <typename T>
    concept Tcp_Response = requires(T a) {
        {
            a.EncodePdu()
        } -> std::same_as<std::string>;
    };

    template <typename T>
    concept Udp_Request = requires(T a){
        { a.DecodePdu(std::declval<const std::string&>()) } -> std::same_as<int>;
    };

    template <typename T>
    concept Udp_Response = requires(T a){
        { a.EncodePdu()} -> std::same_as<std::string>;
    };

}
#endif