#include "error.h"

const char *galay::error::global_err_str[] = {
    "no error",
    "socket function error",
    "send function error",
    "recv function error",
    "ssl_recv function error",
    "ssl_send function error",
    "bind function error",
    "setsockopt function error",
    "listen function error",
    "accept function error",
    "ssl_accept function error",
    "ssl_ctx_new function error",
    "ssl_obj_new function error",
    "ssl certification or server's key file error",
    "choose error engine",
    "connect function error",
    "ssl_connect function error",
    "the getsockopt function in client has some error",
    "scheduler's engine check events error",
    "epoll engine's epoll_wait function has error",
    "protocol is incomplete",
    "the request is a bad request",
};

const char *galay::error::get_err_str(int errcode)
{
    return galay::error::global_err_str[errcode];
}