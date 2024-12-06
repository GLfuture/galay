#include "galay/galay.h"
#include "galay/kernel/EventEngine.h"
#include <iostream>
#include <spdlog/spdlog.h>

#define TEST_DEADLINE

#ifdef TEST_SLEEP
galay::coroutine::Coroutine test()
{
    galay::galay::Timer::ptr timer;
    bool res = co_await galay::this_coroutine::Sleepfor(1000, &timer);
    if(res)
        std::cout << "sleep success" << std::endl;
    else
        std::cout << "sleep failed" << std::endl;
    co_return;
}


int main()
{
#ifdef USE_KQUEUE
    spdlog::set_level(spdlog::level::debug);
    galay::details::KqueueEventEngine::ptr engine = std::make_shared<galay::details::KqueueEventEngine>();
    GHandle handle{};
    galay::details::TimeEvent::CreateHandle(handle);
    
    galay::details::TimeEvent::ptr event = std::make_shared<galay::details::TimeEvent>(handle, engine.get());
    int i = 0;
    galay::galay::Timer::ptr timer = event->AddTimer(1000, [event, &i](galay::galay::Timer::ptr t) {
        std::cout << "timer expired" << std::endl;
        if(i++ < 10) event->ReAddTimer(1000, t);
    });
    
    std::thread th([engine](){
        engine->Loop(-1);
    });
    th.detach();
    getchar();
    engine->Stop();
    getchar(); 
    return 0;
#endif
    galay::InitializeGalayEnv(0 ,1 , 1, -1);
    test();
    std::cout << "main thread wait..." << std::endl;
    getchar();
    galay::DestroyGalayEnv();
    return 0;
}
#elif defined(TEST_DEADLINE)

galay::coroutine::Coroutine* p;

galay::coroutine::Coroutine test()
{
    co_await galay::this_coroutine::GetThisCoroutine(p);
    std::cout << "sleep begin" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(10));
    std::cout << "sleep end" << std::endl;
    co_return;
}

int main()
{ 
    galay::InitializeGalayEnv({1, -1}, {0, -1}, {1, -1});
    std::thread th([]{
        test();
    });
    th.detach();
    getchar();
    std::cout << "ready to resume\n";
    p->Destroy();
    getchar();
    galay::DestroyGalayEnv();

    return 0;
}

#endif