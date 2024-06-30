#include "signalhandler.h"
#include <signal.h>

std::unique_ptr<galay::common::GY_SignalHandler> galay::common::GY_SignalHandler::m_signalHandler;

galay::common::GY_SignalHandler* 
galay::common::GY_SignalHandler::GetInstance()
{
    if(!m_signalHandler){
        m_signalHandler = std::make_unique<GY_SignalHandler>();
    }
    return m_signalHandler.get();
}


void 
galay::common::GY_SignalHandler::SetSignalHandler(int signo, std::function<void(int)> func)
{
    if(!m_signalMap.contains(signo)){
        signal(signo,[](int signo){
            for(auto& func : m_signalHandler->m_signalMap[signo]){
                func(signo);
            }
            signal(signo,SIG_DFL);
            raise(signo);
        });
    }
    m_signalHandler->m_signalMap[signo].push_back(std::move(func));
}