#include "Service.hpp"

namespace galay::details
{

bool RpcWaitAction::HasEventToDo()
{
    return true;
}

bool RpcWaitAction::DoAction(CoroutineBase::wptr co, void *ctx)
{
    return false;
}


}

namespace galay::service 
{
RpcServer::RpcServer(RcpServerConfig config)

{
}

void RpcServer::Start()
{
}


}

