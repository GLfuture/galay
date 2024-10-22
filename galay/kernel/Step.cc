#include "Step.h"

namespace galay
{

std::function<void(GHandle)> AfterCreateTcpSocketStep::m_callback;

void AfterCreateTcpSocketStep::Execute(GHandle handle)
{
    if( m_callback ) m_callback(handle);
}

void AfterCreateTcpSocketStep::Setup(const std::function<void(GHandle)> &callback)
{
    m_callback = callback;
}


}
