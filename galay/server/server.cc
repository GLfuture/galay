#include "server.h"

galay::Server::~Server()
{
    for (auto scheduler : m_schedulers)
    {
        if (!scheduler->is_stop())
        {
            scheduler->stop();
        }
    }
}