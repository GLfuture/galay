#ifndef GALAY_SERVER_HPP
#define GALAY_SERVER_HPP

#include "../kernel/task.h"
#include <functional>
#include <atomic>
#include <type_traits>
#include <vector>
#include <spdlog/spdlog.h>

namespace galay
{
    
    class Server
    {
    public:
        Server(Config::ptr config);

        virtual void start(std::vector<std::pair<uint16_t,std::function<Task<>(Task_Base::wptr)>>> Port_Func) = 0;

        virtual int get_error() ;
    
        virtual void stop();

        virtual Scheduler_Base::ptr get_scheduler(int indx);

        virtual ~Server();

    protected:
        std::vector<int> m_fds;
        Config::ptr m_config;
        int m_error = Error::NoError::GY_SUCCESS; 
        std::vector<Scheduler_Base::ptr> m_schedulers;
    };

    //tcp_server
    template<Tcp_Request REQ,Tcp_Response RESP>
    class Tcp_Server : public Server
    {
    public:
        using uptr = std::unique_ptr<Tcp_Server>;
        Tcp_Server() = delete;
        Tcp_Server(Tcp_Server_Config::ptr config) 
            : Server(config){}

        void start(std::vector<std::pair<uint16_t,std::function<Task<>(Task_Base::wptr)>>> Port_Funcs) override
        {
            if(add_main_tasks(Port_Funcs) == -1) {
                return;
            }
            std::vector<std::thread> threads;
            for(int i = 1 ; i < m_schedulers.size(); i ++)
            {
                threads.push_back(std::thread(&Scheduler_Base::start,m_schedulers[i].get()));
            }
            m_schedulers[0]->start();
            for(int i = 0 ; i < threads.size();i++)
            {
                if(threads[i].joinable()){
                    threads[i].join();
                }
            }
        }

        virtual ~Tcp_Server(){}
    protected:
        virtual int InitSock(uint16_t port) 
        {
            int fd = IOFuntion::TcpFunction::Sock();
            if (fd <= 0)
            {
                spdlog::error("{} {} {} Sock fail {}, close connection",__TIME__,__FILE__,__LINE__,strerror(errno));
                return Error::NetError::GY_SOCKET_ERROR;
            }
            int ret = IOFuntion::TcpFunction::Reuse_Fd(fd);
            if (ret == -1)
            {
                spdlog::error("{} {} {} Reuse_Fd fail {}, close connection",__TIME__,__FILE__,__LINE__,strerror(errno));
                close(fd);
                return Error::NetError::GY_SETSOCKOPT_ERROR;
            }
            ret = IOFuntion::TcpFunction::Bind(fd, port);
            if (ret == -1)
            {
                spdlog::error("{} {} {} Bind fail {}, close connection",__TIME__,__FILE__,__LINE__,strerror(errno));
                close(fd);
                return Error::NetError::GY_BIND_ERROR;
            }
            ret = IOFuntion::TcpFunction::Listen(fd, std::dynamic_pointer_cast<Tcp_Server_Config>(this->m_config)->m_backlog);
            if (ret == -1)
            {
                spdlog::error("{} {} {} Listen fail {}, close connection",__TIME__,__FILE__,__LINE__,strerror(errno));
                close(fd);
                return Error::NetError::GY_LISTEN_ERROR;
            }
            IOFuntion::TcpFunction::IO_Set_No_Block(fd);
            this->m_fds.push_back(fd); 
            return Error::NoError::GY_SUCCESS;
        }

        virtual int add_main_tasks(std::vector<std::pair<uint16_t,std::function<Task<>(Task_Base::wptr)>>> Port_Func)
        {
            int n = Port_Func.size();
            for(int i = 0 ; i < n ; ++i){
                if(InitSock(Port_Func[i].first) != Error::NoError::GY_SUCCESS) {
                    for(auto& v: m_fds) {
                        this->m_schedulers[0]->del_event(v,GY_EVENT_READ | GY_EVENT_WRITE | GY_EVENT_ERROR);
                        close(v);
                        return -1;
                    }
                }
                auto config = std::dynamic_pointer_cast<Tcp_Server_Config>(this->m_config);
                std::vector<std::weak_ptr<Scheduler_Base>> schedulers;
                for(auto scheduler: m_schedulers)
                {
                    schedulers.push_back(scheduler);
                }
                auto task = std::make_shared<Tcp_Main_Task<REQ,RESP>>(this->m_fds[i],schedulers, std::move(Port_Func[i].second), config->m_max_rbuffer_len,config->m_conn_timeout);
                if (config->m_keepalive_conf.m_keepalive)
                {
                    task->enable_keepalive(config->m_keepalive_conf.m_idle, config->m_keepalive_conf.m_interval, config->m_keepalive_conf.m_retry);
                }
                this->m_schedulers[0]->add_task(std::make_pair(this->m_fds[i], task));
                if(this->m_schedulers[0]->add_event(this->m_fds[i], GY_EVENT_READ | GY_EVENT_ERROR)==-1) {
                    spdlog::error("{} {} {} scheduler add event fail(fd: {} ) {}, close connection",__TIME__,__FILE__,__LINE__,this->m_fds[i],strerror(errno));
                    for(auto& v: m_fds) {
                        this->m_schedulers[0]->del_event(v,GY_EVENT_READ | GY_EVENT_WRITE | GY_EVENT_ERROR);
                        close(v);
                        return -1;
                    }
                }
            }
            return 0;
        }
    };

    template<Tcp_Request REQ,Tcp_Response RESP>
    class Tcp_SSL_Server: public Tcp_Server<REQ,RESP>
    {
    public:
        using uptr = std::unique_ptr<Tcp_SSL_Server>;
        Tcp_SSL_Server(Tcp_SSL_Server_Config::ptr config)
            : Tcp_Server<REQ,RESP>(config)
        {
            m_ctx = IOFuntion::TcpFunction::SSL_Init_Server(config->m_ssl_min_version, config->m_ssl_max_version);
            if (m_ctx == nullptr)
            {
                this->m_error = Error::NetError::GY_SSL_CTX_INIT_ERROR;
                spdlog::error("{} {} {} {}",__TIME__,__FILE__,__LINE__,strerror(errno));
                exit(-1);
            }
            if (IOFuntion::TcpFunction::SSL_Config_Cert_And_Key(m_ctx, config->m_cert_filepath.c_str(), config->m_key_filepath.c_str()) == -1)
            {
                this->m_error = Error::NetError::GY_SSL_CRT_OR_KEY_FILE_ERROR;
                spdlog::error("{} {} {} SSL_Config_Cert_And_Key: {}",__TIME__,__FILE__,__LINE__,strerror(errno));
                exit(-1);
            }
        }

        ~Tcp_SSL_Server() override
        {
            if (this->m_ctx)
            {
                IOFuntion::TcpFunction::SSLDestory({}, m_ctx);
                this->m_ctx = nullptr;
            }
        }

    protected:
        int add_main_tasks(std::vector<std::pair<uint16_t,std::function<Task<>(Task_Base::wptr)>>> Port_Funcs) override
        {
            int n = Port_Funcs.size();
            for(int i = 0 ; i < n ; ++ i){
                if(Tcp_Server<REQ,RESP>::InitSock(Port_Funcs[i].first) != Error::NoError::GY_SUCCESS) {
                    for(auto& v: this->m_fds) {
                        this->m_schedulers[0]->del_event(v,GY_EVENT_READ | GY_EVENT_WRITE | GY_EVENT_ERROR);
                        close(v);
                        return -1;
                    }
                }
                std::vector<std::weak_ptr<Scheduler_Base>> schedulers;
                for(auto scheduler: this->m_schedulers)
                {
                    schedulers.push_back(scheduler);
                }
                Tcp_SSL_Server_Config::ptr config = std::dynamic_pointer_cast<Tcp_SSL_Server_Config>(this->m_config);
                auto task = std::make_shared<Tcp_SSL_Main_Task<REQ,RESP>>(this->m_fds[i], schedulers, std::move(Port_Funcs[i].second), config->m_max_rbuffer_len ,config->m_conn_timeout, config->m_ssl_accept_retry, this->m_ctx);
                if (config->m_keepalive_conf.m_keepalive)
                {
                    task->enable_keepalive(config->m_keepalive_conf.m_idle, config->m_keepalive_conf.m_interval, config->m_keepalive_conf.m_retry);
                }
                this->m_schedulers[0]->add_task(std::make_pair(this->m_fds[i], task));
                if(this->m_schedulers[0]->add_event(this->m_fds[i], GY_EVENT_READ | GY_EVENT_ERROR)==-1) {
                    spdlog::error("{} {} {} scheduler add event fail(fd: {} ) {}, close connection",__TIME__,__FILE__,__LINE__,this->m_fds[i],strerror(errno));
                    for(auto& v: this->m_fds) {
                        this->m_schedulers[0]->del_event(v,GY_EVENT_READ | GY_EVENT_WRITE | GY_EVENT_ERROR);
                        close(v);
                        return -1;
                    }
                }
            }
            return 0;
        }

    protected:
        SSL_CTX *m_ctx = nullptr;
    };

    // to do


    template<Tcp_Request REQ,Tcp_Response RESP>
    class Udp_Server : public Server
    {
    public:
        //创建时socket
        Udp_Server(Udp_Server_Config::ptr config)
            :Server(config)
        {
            // for(int i = 0 ; i < config->m_ports.size() ; i ++){
            //     int fd = IOFuntion::UdpFunction::Sock();
            //     IOFuntion::BlockFuction::IO_Set_No_Block(fd);
            //     IOFuntion::UdpFunction::Bind(fd,config->m_ports[i]);
            //     m_schedulers[0]->add_event(fd,GY_EVENT_READ);
            //     this->m_fds.push_back(fd);
            // }
            
        }

        void start(std::vector<std::function<Task<>(std::weak_ptr<Task_Base>)>> funcs) override
        {

        }
    };

}

#endif