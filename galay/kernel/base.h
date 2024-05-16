#ifndef GALAY_CONCEPTS_H
#define GALAY_CONCEPTS_H
#include <string>
#include <concepts>

namespace galay
{
    enum ProtoJudgeType {
        PROTOCOL_FINISHED,
        PROTOCOL_INCOMPLETE,
        PROTOCOL_ILLEGAL
    };

    template <typename T>
    concept Tcp_Request = requires(T a) {
        { 
            //会附带解析头部的职责
            a.DecodePdu(::std::declval<::std::string&>())
        } -> ::std::same_as<ProtoJudgeType>;

        { 
            a.Clear()
        } -> ::std::same_as<void>;
    };
    template <typename T>
    concept Tcp_Response = requires(T a) {
        {
            a.EncodePdu()
        } -> ::std::same_as<::std::string>;

        { 
            a.Clear()
        } -> ::std::same_as<void>;
    };

    template <typename T>
    concept Udp_Request = requires(T a){
        { 
            a.DecodePdu(::std::declval<::std::string&>()) 
        } -> ::std::same_as<int>;

        { 
            a.Clear() 
        } -> ::std::same_as<void>;
    };

    template <typename T>
    concept Udp_Response = requires(T a){
        {
            a.EncodePdu()
        } -> ::std::same_as<::std::string>;

        { 
            a.Clear() 
        } -> ::std::same_as<void>;
    };

}
#endif