#include "signalhandler.h"
#include <signal.h>

std::unique_ptr<galay::common::GY_SignalFactory> galay::common::GY_SignalFactory::m_signalHandler;

galay::common::GY_SignalFactory* 
galay::common::GY_SignalFactory::GetInstance()
{
    if(!m_signalHandler){
        m_signalHandler = std::make_unique<GY_SignalFactory>();
    }
    return m_signalHandler.get();
}


void 
galay::common::GY_SignalFactory::SetSignalHandler(int signo, std::function<void(int)> func)
{
    if(!m_signalMap.contains(signo)){
        signal(signo,[](int signo){
            for(auto& func : m_signalHandler->m_signalMap[signo]){
                func(signo);
            }
        });
    }
    m_signalHandler->m_signalMap[signo].push_back(std::move(func));
}