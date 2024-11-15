#include "galay.h"
#include "kernel/EventEngine.h"
#include <iostream>
#include <spdlog/spdlog.h>

int main()
{
#ifdef USE_KQUEUE
    spdlog::set_level(spdlog::level::debug);
    galay::event::KqueueEventEngine::ptr engine = std::make_shared<galay::event::KqueueEventEngine>();
    GHandle handle;
    galay::event::TimeEvent::CreateHandle(handle);
    
    galay::event::TimeEvent::ptr event = std::make_shared<galay::event::TimeEvent>(handle, engine.get());
    int i = 0;
    galay::event::Timer::ptr timer = event->AddTimer(1000, [event, &i](galay::event::Timer::ptr t) {
        std::cout << "timer expired" << std::endl;
        if(i++ < 10) event->ReAddTimer(1000, t);
    });
    
    std::thread th([engine](){
        engine->Loop();
    });
    th.detach();
    getchar();
    engine->Stop();
    getchar(); 
    return 0;
#endif
}