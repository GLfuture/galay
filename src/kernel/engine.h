#ifndef GALAY_ENGINE_H
#define GALAY_ENGINE_H

#include <vector>
#ifdef __linux__
#include <sys/epoll.h>
#include <unistd.h>
#endif
#include <sys/select.h>
#include <memory>
#include "error.h"

namespace galay
{
#ifdef __linux__
    class Engine
    {
    public:
        using ptr = std::shared_ptr<Engine>;
        virtual int event_check() = 0; // if return 0 is success , return < 0 is failed
        virtual int get_active_event_num() const = 0;
        virtual int add_event(int fd , uint32_t event_type) = 0;
        virtual int del_event(int fd , uint32_t event_type) = 0;
        virtual int mod_event(int fd , uint32_t event_type) = 0;
        virtual void* result() = 0;
        virtual int get_error()
        {
            return this->m_error;
        }
        virtual void stop() = 0;
        virtual ~Engine() {}
    protected:
        int m_error = error::base_error::GY_SUCCESS;
    };


    class Epoll_Engine :public Engine
    {
    public:
        using ptr = std::shared_ptr<Epoll_Engine>;
        explicit Epoll_Engine(int max_events, int time_out);

        int event_check() override;

        // return evnents , combine with event_check
        void *result() override;

        // return epollfd
        int get_epoll_fd() const;

        // return events' size
        int get_event_size() const;
        
        //get epoll_wait return
        int get_active_event_num() const override;

        int add_event(int fd ,uint32_t event_type) override;
        
        int del_event(int fd, uint32_t event_type) override;

        int mod_event(int fd, uint32_t event_type) override;

        void stop() override;

        ~Epoll_Engine() ;
    private:
        int m_epfd = 0;
        epoll_event *m_events = nullptr;
        int m_events_size;
        int m_time_out;
        int m_active_event_num = 0;
    };
#endif
    class Select_Engine
    {
    public:
        using ptr = std::shared_ptr<Select_Engine>;
        Select_Engine()
        {

        }
    };
};
#endif