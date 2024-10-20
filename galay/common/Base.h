#ifndef __GALAY_BASE_H__
#define __GALAY_BASE_H__

#if defined(__linux__)
typedef int GHandle;
#elif defined(__APPLE__)
typedef int GHandle;
#elif defined(WIN32) || defined(_WIN32) || defined(_WIN32_) || defined(WIN64) || defined(_WIN64) || defined(_WIN64_)
#include <WinSock2.h>
#pragma comment(lib,"ws2_32.lib")
typedef SOCKET GHandle;
typedef int socklen_t;
typedef signed long ssize_t;

#else
#error "Unsupported platform"
#endif

#if defined(__linux__)
    //#include <linux/version.h>
    //#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,5,0)
    //    #define USE_IOURING
    //#else
        #define USE_EPOLL
    //#endif
    #define closesocket(x) close(x)
#elif defined(WIN32) || defined(_WIN32) || defined(_WIN32_) || defined(WIN64) || defined(_WIN64) || defined(_WIN64_)
    #define USE_IOCP
#endif

#if defined(USE_EPOLL)
    #include <sys/epoll.h>
    #include <sys/eventfd.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
#elif defined(USE_IOURING)
    #include <liburing.h>
#elif defined(USE_IOCP)
    
#endif

#endif