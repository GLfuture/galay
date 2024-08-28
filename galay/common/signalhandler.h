#ifndef GALAY_SIGNALHANDLER_H
#define GALAY_SIGNALHANDLER_H

#include <memory>
#include <functional>
#include <unordered_map>
#include <list>

namespace galay
{
    namespace common
    {
        class SignalFactory
        {
        public:
            static SignalFactory* GetInstance();
            void SetSignalHandler(int signo, std::function<void(int)> func);
        private:
            static std::unique_ptr<SignalFactory> m_signalHandler;
            std::unordered_map<int,std::list<std::function<void(int)>> > m_signalMap;
        };
    }
}




#endif