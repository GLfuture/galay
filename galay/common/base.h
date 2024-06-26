#ifndef GALAY_CONCEPTS_H
#define GALAY_CONCEPTS_H
#include <string>
#include <concepts>
#include <memory>
#include "../common/coroutine.h"


namespace galay
{
    namespace kernel
    {
        class GY_Controller;
        class GY_HttpController;
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
#define DEFAULT_RBUFFER_LENGTH 1024
#define DEFAULT_EVENT_SIZE 1024

#define DEFAULT_SSL_MIN_VERSION TLS1_2_VERSION
#define DEFAULT_SSL_MAX_VERSION TLS1_2_VERSION

#define DEFAULT_SSL_CERT_PATH "ssl/server.crt"
#define DEFAULT_SSL_KEY_PATH "ssl/server.key"

#define DNS_QUERY_ID_MAX 1000

        enum ProtoJudgeType
        {
            kProtoFinished,
            kProtoIncomplete,
            kProtoIllegal
        };

        template <typename T>
        concept TcpRequest = requires(T a) {
            {
                // 会附带解析头部的职责
                a.DecodePdu(std::declval<std::string &>())
            } -> std::same_as<ProtoJudgeType>;

            {
                a.Clear()
            } -> std::same_as<void>;
        };
        template <typename T>
        concept TcpResponse = requires(T a) {
            {
                a.EncodePdu()
            } -> std::same_as<std::string>;

            {
                a.Clear()
            } -> std::same_as<void>;
        };

        template <typename T>
        concept UdpRequest = requires(T a) {
            {
                a.DecodePdu(std::declval<std::string &>())
            } -> std::same_as<ProtoJudgeType>;

            {
                a.Clear()
            } -> std::same_as<void>;
        };

        template <typename T>
        concept UdpResponse = requires(T a) {
            {
                a.EncodePdu()
            } -> std::same_as<std::string>;

            {
                a.Clear()
            } -> std::same_as<void>;
        };

        template <typename T>
        concept TcpCoreType = requires(T a) {
            {
                //To do
                a.CoreBusiness(std::declval<std::weak_ptr<kernel::GY_Controller>>)
            } -> std::same_as<GY_TcpCoroutine<CoroutineStatus>>;
        };

        template <typename T>
        concept HttpCoreType = requires(T a) {
            {
                //To do
                a.CoreBusiness(std::declval<std::weak_ptr<kernel::GY_HttpController>>)
            } -> std::same_as<GY_TcpCoroutine<CoroutineStatus>>;
        };

        enum SchedulerType
        {
            kSelectScheduler, // select
            kEpollScheduler,  // epoll
        };

        enum ClassNameType
        {
            kClassNameRequest,  // protocol request 
            kClassNameResponse, // protocol response 
            kClassNameCoreBusinuess,    //核心业务类
        };
    }
}
#endif