#include "server.h"

galay::Server::Server(Config::ptr config)
    : m_config(config)
{
    switch (config->m_type)
    {
    case Engine_Type::ENGINE_EPOLL:
    {
        for (int i = 0; i < config->m_threadnum; i++)
        {
            this->m_schedulers.push_back(std::make_shared<Epoll_Scheduler>(config->m_max_event_size, config->m_sche_wait_time));
        }
    }
    break;
    case Engine_Type::ENGINE_SELECT:
    {
        for (int i = 0; i < config->m_threadnum; i++)
        {
            this->m_schedulers.push_back(std::make_shared<Select_Scheduler>(config->m_sche_wait_time));
        }
    }
    break;
    default:
        break;
    }
}

void galay::Server::Stop()
{
    for (auto scheduler : m_schedulers)
    {
        scheduler->Stop();
    }
}

int galay::Server::GetError() 
{ 
    return this->m_error; 
}

galay::Scheduler_Base::ptr galay::Server::GetScheduler(int indx) 
{ 
    return this->m_schedulers[indx]; 
}

galay::Server::~Server()
{
    for (auto scheduler : m_schedulers)
    {
        if (!scheduler->IsStop())
        {
            scheduler->Stop();
        }
    }
}