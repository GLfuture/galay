#ifndef GALAY_CONCEPTS_H
#define GALAY_CONCEPTS_H
#include <string>
#include <concepts>
#include <memory>
#include "../protocol/basic_protocol.h"


namespace galay
{
    namespace kernel
    {
        class GY_Controller;
        class GY_HttpController;
    }
    
    namespace html
    {
        extern const char* Html404NotFound;
        extern const char* Html405MethodNotAllowed;
    }

    namespace protocol
    {
        enum ProtoJudgeType;
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

#define DEFAULT_COROUTINE_POOL_THREADNUM 4

        

        template <typename T>
        concept TcpRequest = requires(T a) {
            {
                // 会附带解析头部的职责
                a.DecodePdu(std::declval<std::string &>())
            } -> std::same_as<protocol::ProtoJudgeType>;
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
                a.DecodePdu(std::declval<std::string &>())
            } -> std::same_as<protocol::ProtoJudgeType>;
        };

        template <typename T>
        concept UdpResponse = requires(T a) {
            {
                a.EncodePdu()
            } -> std::same_as<std::string>;
        };
        
        enum SchedulerType
        {
            kSelectScheduler, // select
            kEpollScheduler,  // epoll
        };

        enum ClassNameType
        {
            kClassNameRequest,          // protocol request 
            kClassNameResponse,         // protocol response 
            kClassNameCoreBusinuess,    //核心业务类
        };

        enum CoroutineStatus
        {
            kCoroutineNotExist,       // 协程不存在
            kCoroutineRunning,        // 协程运行
            kCoroutineSuspend,        // 协程挂起
            kCoroutineFinished,       // 协程结束
            kCoroutineWaitingForData, // 正在等待数据
        };
    }
}
#endif