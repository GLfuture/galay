// #include "galay/galay.h"
// #include "galay/kernel/EventEngine.h"
// #include <iostream>
// #include <spdlog/spdlog.h>

// galay::coroutine::Coroutine test()
// {
//     galay::event::Timer::ptr timer;
//     bool res = co_await galay::this_coroutine::SleepFor(1000, &timer);
//     if(res)
//         std::cout << "sleep success" << std::endl;
//     else
//         std::cout << "sleep failed" << std::endl;
//     co_return;
// }


// int main()
// {
// #ifdef USE_KQUEUE
//     spdlog::set_level(spdlog::level::debug);
//     galay::event::KqueueEventEngine::ptr engine = std::make_shared<galay::event::KqueueEventEngine>();
//     GHandle handle{};
//     galay::event::TimeEvent::CreateHandle(handle);
    
//     galay::event::TimeEvent::ptr event = std::make_shared<galay::event::TimeEvent>(handle, engine.get());
//     int i = 0;
//     galay::event::Timer::ptr timer = event->AddTimer(1000, [event, &i](galay::event::Timer::ptr t) {
//         std::cout << "timer expired" << std::endl;
//         if(i++ < 10) event->ReAddTimer(1000, t);
//     });
    
//     std::thread th([engine](){
//         engine->Loop(-1);
//     });
//     th.detach();
//     getchar();
//     engine->Stop();
//     getchar(); 
//     return 0;
// #endif
//     galay::DynamicResizeTimerSchedulers(1);
//     galay::DynamicResizeCoroutineSchedulers(1);
//     galay::StartCoroutineSchedulers();
//     galay::StartTimerSchedulers(-1);
//     test();
//     std::cout << "main thread wait..." << std::endl;
//     getchar();
//     galay::StopTimerSchedulers();
//     galay::StopCoroutineSchedulers();
//     galay::GetCoroutineStore()->Clear();
//     return 0;
// }

#include "galay/galay.h"
#include <iostream>


int main()
{
    {
        galay::IOVecHolder holder(1024);
        std::cout << (void*)holder->m_buffer << std::endl;
        holder.Reset();
        std::cout << (void*)holder->m_buffer << std::endl;
    }

    {
        galay::IOVecHolder holder(1024);
        std::cout << (void*)holder->m_buffer << std::endl;
        galay::IOVecHolder holder2;
        holder2.Reset(std::string("Hello World"));
        std::cout << (void*)holder2->m_buffer << std::endl;
    }

    {
        galay::IOVecHolder holder;
        holder.Reset(std::string("Hello World"));
        std::cout << (void*)holder->m_buffer << std::endl;
    }
    return 0;
}