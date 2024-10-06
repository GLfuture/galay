#ifndef GALAY_CONCEPTS_H
#define GALAY_CONCEPTS_H
#include <string>
#include <concepts>
#include <memory>
#include "../protocol/protocol.h"


namespace galay
{
    namespace html
    {
        extern const char* Html404NotFound;
        extern const char* Html405MethodNotAllowed;
    }

    namespace common
    {
#define DEFAULT_BACKLOG 128
#define DEFAULT_MAX_SSL_ACCEPT_RETRY 1000
#define DEFAULT_SSL_SLEEP_MISC_PER_RETRY 1
#define DEFAULT_CLINET_EVENT_SIZE 1
#define MAX_UDP_WAIT_FOR_RECV_TIME 5000
#define DEFAULT_CONNCTION_TIMEOUT -1
#define DEFAULT_LISTEN_PORT 8080

#define DEFAULT_THREAD_NUM 4
#define DEFAULT_SCHE_WAIT_TIME 5
#define DEFAULT_RBUFFER_LENGTH 256 * 1024
#define DEFAULT_EVENT_SIZE 1024

#define DNS_QUERY_ID_MAX 1000

#define DEFAULT_COROUTINE_POOL_THREADNUM 4

        

        template <typename T>
        concept TcpRequest = requires(T a) {
            {
                // 会附带解析头部的职责
                a.DecodePdu(std::declval<const std::string&>())
            } -> std::same_as<int>;
        };
        template <typename T>
        concept TcpResponse = requires(T a) {
            {
                a.EncodePdu()
            } -> std::same_as<std::string>;
        };

        template <typename T>
        concept UdpRequest = requires(T a) {
            {
                a.DecodePdu(std::declval<const std::string &>())
            } -> std::same_as<int>;
        };

        template <typename T>
        concept UdpResponse = requires(T a) {
            {
                a.EncodePdu()
            } -> std::same_as<std::string>;
        };

    }
}
#endif