#ifndef GALAY_GLOBAL_H
#define GALAY_GLOBAL_H
#include "factory.h"
namespace galay {

    #define DEFAULT_BACKLOG                         128        
    #define DEFAULT_MAX_SSL_ACCEPT_RETRY            1000
    #define DEFAULT_SSL_SLEEP_MISC_PER_RETRY        1
    #define DEFAULT_CLINET_EVENT_SIZE               1
    #define MAX_UDP_WAIT_FOR_RECV_TIME              5000
    #define DEFAULT_CONNCTION_TIMEOUT               -1
    
    
    #define DEFAULT_THREAD_NUM                      4
    #define DEFAULT_SCHE_WAIT_TIME                  5
    #define DEFAULT_RBUFFER_LENGTH                  1024
    #define DEFAULT_EVENT_SIZE                      1024

    #define DEFAULT_SSL_MIN_VERSION                 TLS1_2_VERSION
    #define DEFAULT_SSL_MAX_VERSION                 TLS1_2_VERSION

    #define DEFAULT_SSL_CERT_PATH                   "ssl/server.crt"
    #define DEFAULT_SSL_KEY_PATH                    "ssl/server.key"

    #define DNS_QUERY_ID_MAX                        1000

    extern galay::GY_TcpProtocolFactoryBase::uptr g_tcp_protocol_factory;
}
#endif