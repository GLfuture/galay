#ifndef __GALAY_BASE_H__
#define __GALAY_BASE_H__

#if defined(__linux__)
struct GHandle {
    int fd;
};
#elif defined(__APPLE__)
struct GHandle {
    int fd;
};
#elif defined(WIN32) || defined(_WIN32) || defined(_WIN32_) || defined(WIN64) || defined(_WIN64) || defined(_WIN64_)
#include <WinSock2.h>
#pragma comment(lib,"ws2_32.lib")
struct GHandle {
    SOCKET fd;
};
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
#elif defined(WIN32) || defined(_WIN32) || defined(_WIN32_) || defined(WIN64) || defined(_WIN64) || defined(_WIN64_)
    #define USE_IOCP
    #define close(x) closesocket(x)
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__)
    #define USE_KQUEUE
#else
    #error "Unsupported platform"
#endif

#if defined(USE_EPOLL)
    #include <sys/epoll.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
#elif defined(USE_IOURING)
    #include <liburing.h>
#elif defined(USE_IOCP)

#elif defined(USE_KQUEUE)
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <sys/event.h>
    
#endif
#define GALAY_EXTERN_API __attribute__((visibility("default")))
#endif