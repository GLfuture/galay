#ifndef GALAY_SERVER_H
#define GALAY_SERVER_H

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

        virtual void start(std::function<void(std::shared_ptr<REQ> request, std::shared_ptr<RESP> response)> &&func) = 0;

        virtual void protocol_proccess(std::string &rbuffer, std::string &wbuffer,
                                       std::function<void(std::shared_ptr<REQ> request, std::shared_ptr<RESP> response)> &&func) = 0;

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
        int m_error = error::server_error::GY_SUCCESS;
        std::atomic_bool m_stop;
        Engine::ptr m_engine;
        std::unordered_map<int, Task::ptr> m_tasks;
    };

    
    // tcp server
    template <Request REQ, Response RESP>
    class Tcp_Server : public Server<REQ, RESP>
    {
    public:
        using ptr = std::shared_ptr<Tcp_Server>;
        Tcp_Server() = delete;
        Tcp_Server(Tcp_Server_Config::ptr config) : Server<REQ, RESP>(config)
        {
        }

        Tcp_Server(Tcp_Server_Config &&config) : Server<REQ, RESP>(std::make_shared<Tcp_Server_Config>(config))
        {
        }

        void start(std::function<void(std::shared_ptr<REQ> request, std::shared_ptr<RESP> response)> &&func) override
        {
            Tcp_Server_Config::ptr config = std::dynamic_pointer_cast<Tcp_Server_Config>(this->m_config);
            this->m_error = init(this->m_fd, config);
            if (this->m_error != error::server_error::GY_SUCCESS)
                return;
            switch (config->m_engine)
            {
            case IO_ENGINE::IO_POLL:
            {
                break;
            }
            case IO_ENGINE::IO_SELECT:
                break;
            case IO_ENGINE::IO_EPOLL:
            {
                this->m_engine = std::make_shared<Epoll_Engine>(config->m_event_size, config->m_event_time_out);
                Epoll_Engine::ptr engine = std::dynamic_pointer_cast<Epoll_Engine>(this->m_engine);
                engine->add_event(this->m_fd,EPOLLIN | EPOLLET);
                while (!this->m_stop)
                {
                    int ret = engine->event_check();
                    if (ret == -1)
                    {
                        //need to call engine's get_error
                        this->m_error = error::server_error::GY_ENGINE_HAS_ERROR;
                        return;
                    }
                    epoll_event *events = engine->result();
                    int nready = engine->get_active_event_num();
                    for (int i = 0; i < nready; i++)
                    {
                        int fd = events[i].data.fd;
                        if (fd == this->m_fd)
                        {
                            epoll_accept_conn(engine, this->m_fd);
                        }
                        else if (events[i].events & EPOLLIN)
                        {
                            Tcp_Task::ptr task = std::dynamic_pointer_cast<Tcp_Task>(this->m_tasks.at(fd));
                            std::string& rbuffer = task->Get_Rbuffer();
                            if (epoll_read_package(engine, config, fd, rbuffer) == -1) continue; 
                            std::string& wbuffer = task->Get_Wbuffer();
                            protocol_proccess(rbuffer, wbuffer,std::forward<std::function<void(std::shared_ptr<REQ> request, std::shared_ptr<RESP> response)>>(func));
                            engine->mod_event(fd, EPOLLOUT);
                        }
                        else if (events[i].events & EPOLLOUT)
                        {
                            Tcp_Task::ptr task = std::dynamic_pointer_cast<Tcp_Task>(this->m_tasks.at(fd));
                            std::string& wbuffer = task->Get_Wbuffer();
                            epoll_send_package(engine, fd, wbuffer);
                            engine->mod_event(events[i].data.fd, EPOLLIN);
                        }
                    }
                }
                break;
            }
            case IO_ENGINE::IO_URING:
                break;
            default:
                this->m_error = error::server_error::GY_ENGINE_CHOOSE_ERROR;
                break;
            }
        }

        ~Tcp_Server()
        {
            for(auto it = this->m_tasks.begin() ; it != this->m_tasks.end() ; it++)
            {
                this->m_engine->del_event(it->first,EPOLLIN);
                close(it->first);
            }
            this->m_tasks.clear();
        }

    protected:
        int init(int &sockfd, Tcp_Server_Config::ptr config)
        {
            sockfd = iofunction::Tcp_Function::Sock();
            if (sockfd <= 0)
            {
                return error::server_error::GY_SOCKET_ERROR;
            }
            int ret = iofunction::Tcp_Function::Bind(sockfd, config->m_port);
            if (ret == -1)
            {
                close(sockfd);
                return error::server_error::GY_BIND_ERROR;
            }
            ret = iofunction::Tcp_Function::Listen(sockfd, config->m_backlog);
            if (ret == -1)
            {
                close(sockfd);
                return error::server_error::GY_LISTEN_ERROR;
            }
            iofunction::Tcp_Function::IO_Set_No_Block(sockfd);
            return error::server_error::GY_SUCCESS;
        }

        void epoll_accept_conn(Epoll_Engine::ptr engine, int fd)
        {
            int connfd = iofunction::Tcp_Function::Accept(fd);
            if (connfd <= 0)
                return;
            iofunction::Tcp_Function::IO_Set_No_Block(fd);
            this->m_tasks.emplace(connfd, std::make_shared<Tcp_Task>());
            engine->add_event(connfd, EPOLLIN|EPOLLET);
        }

        int epoll_read_package(Epoll_Engine::ptr engine, Tcp_Server_Config::ptr config, int fd, std::string &buffer)
        {
            int len = iofunction::Tcp_Function::Recv(fd, buffer, config->m_recv_len);
            if (len == 0)
            {
                close(fd);
                engine->del_event(fd, EPOLLIN);
                this->m_tasks.erase(fd);
                return -1;
            }
            else if (len == -1)
            {
                if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
                {
                    return 0;
                }
                else
                {
                    close(fd);
                    engine->del_event(fd, EPOLLIN);
                    this->m_tasks.erase(fd);
                    return -1;
                }
            }
            return 0;
        }

        void epoll_send_package(Epoll_Engine::ptr engine, int fd, std::string &wbuffer)
        {
            int len = iofunction::Tcp_Function::Send(fd, wbuffer, wbuffer.length());
            if (len == -1)
            {
                if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
                {
                    return;
                }
                else
                {
                    close(fd);
                    engine->del_event(fd, EPOLLIN);
                    this->m_tasks.erase(fd);
                    return;
                }
            }
            wbuffer.erase(wbuffer.begin(), wbuffer.begin() + len);
        }

        void protocol_proccess(std::string &rbuffer, std::string &wbuffer,
                               std::function<void(std::shared_ptr<REQ> request, std::shared_ptr<RESP> response)> &&func) override
        {
            std::shared_ptr<REQ> req = std::make_shared<REQ>();
            int state = error::package_error::GY_PACKAGE_SUCCESS;
            int len = req->decode(rbuffer, state);
            if (state == error::package_error::GY_PACKAGE_INCOMPLETE)
                return;
            rbuffer.erase(rbuffer.begin(), rbuffer.begin() + len);
            std::shared_ptr<RESP> resp = std::make_shared<RESP>();
            func(req, resp);
            wbuffer = resp->encode();
        }

    protected:
        SSL_CTX *m_ctx;
    };

    // to do
    template <Request REQ, Response RESP>
    class Udp_Server : public Server<REQ, RESP>
    {
    public:
    };

}

#endif