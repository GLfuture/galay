#include "error.h"

const char *galay::Error::global_err_str[] = {
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
    "connect function error",
    "ssl_connect function error",
    "the getsockopt function in client has some error",
    "sendto function error",
    "recvfrom function error",
    "set no block error",
    "scheduler engine choose error",
    "incorrect function size",
    "scheduler's engine check events error",
    "scheduler is expired",
    "scheduler modify event's status fail",
    "protocol is incomplete",
    "the request is a bad request",
    "protocol is illegal",
};

const char *galay::Error::get_err_str(int errcode)
{
    return galay::Error::global_err_str[errcode];
}