#ifndef GALAY_SERVER_HPP
#define GALAY_SERVER_HPP

#include "../config/config.h"
#include "../kernel/iofunction.h"
#include "../kernel/error.h"
#include "../kernel/engine.h"
#include "../kernel/basic_concepts.h"
#include "../kernel/task.h"
#include <functional>
#include <concepts>
#include <unordered_map>
#include <atomic>
#include <type_traits>

namespace galay
{
    // server
    template <Request REQ, Response RESP>
    class Server
    {
    public:
        Server(Config::ptr config) : m_config(config)
        {
            m_stop.store(false, std::memory_order::relaxed);
        }

        virtual void start(std::function<void(std::shared_ptr<Task<REQ,RESP>>)> &&func) = 0;

        virtual int get_error()
        {
            return this->m_error;
        }

        virtual void stop()
        {
            this->m_stop.store(true, std::memory_order::relaxed);
        }

        Engine::ptr get_engine()
        {
            return this->m_engine;
        }

        virtual ~Server()
        {
            this->m_engine->stop();
        }

    protected:
        int m_fd = 0;
        Config::ptr m_config;
        int m_error = error::base_error::GY_SUCCESS;
        std::atomic_bool m_stop;
        Engine::ptr m_engine;
        std::unordered_map<int, std::shared_ptr<Task<REQ,RESP>>> m_tasks;
    };

    
    
    // to do
    template <Request REQ, Response RESP>
    class Udp_Server : public Server<REQ, RESP>
    {
    public:
    };

}

#endif