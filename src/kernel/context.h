#ifndef GALAY_CONTEXT_H
#define GALAY_CONTEXT_H

#include "../protocol/tcp.h"
#include "../protocol/http.h"
#include "iofunction.h"
#include <memory>

namespace galay
{
    class Task_Context
    {
    public:
        using ptr = std::shared_ptr<Task_Context>;
    };


    class Tcp_Task_Context
    {
    public:
        using ptr = std::shared_ptr<Tcp_Task_Context>;
        Tcp_Request::ptr req;
        Tcp_Response::ptr resp;
        iofunction::Addr addr;
    };
    

};

#endif