#include "server.h"

galay::Server::~Server()
{
    if (!this->m_scheduler->is_stop())
    {
        this->m_scheduler->stop();
    }
}