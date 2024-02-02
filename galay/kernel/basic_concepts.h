#ifndef GALAY_CONCEPTS_H
#define GALAY_CONCEPTS_H
#include <string>
#include <concepts>

namespace galay
{
    template <typename T>
    concept Tcp_Request = requires(T a) {
        { a.decode(std::declval<const std::string&>(),std::declval<int&>()) } -> std::same_as<int>;
        { a.proto_type() } -> std::same_as<int>;//
        //返回需要读取的长度，GY_ALL_RECIEVE_PROTOCOL_TYPE无需在意，GY_PACKAGE_FIXED_PROTOCOL_TYPE返回固定值，GY_HEAD_FIXED_PROTOCOL_TYPE返回头固定长度
        { a.proto_fixed_len() } -> std::same_as<int>;
        // GY_HEAD_FIXED_PROTOCOL_TYPE 返回body的长度
        { a.proto_extra_len() } -> std::same_as<int>;
        { a.set_extra_msg(std::declval<std::string&&>()) } -> std::same_as<void>;
    };
    template <typename T>
    concept Tcp_Response = requires(T a) {
        {
            a.encode()
        } -> std::same_as<std::string>;
    };

    template <typename T>
    concept Udp_Request = requires(T a){
        { a.encode() } -> std::same_as<std::string>;
    };

    template <typename T>
    concept Udp_Response = requires(T a){
        { a.decode(std::declval<const std::string&>())} -> std::same_as<int>;
    };

}
#endif