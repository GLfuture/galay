#include "galay/galay.h"
#include "galay/kernel/EventEngine.h"
#include <iostream>

#define TEST_AUTO_DESTROY

#ifdef TEST_SLEEP
galay::Coroutine test()
{
    std::cout << "start" << std::endl;
    co_await galay::this_coroutine::Sleepfor(5000);
    std::cout << "sleep 1s" << std::endl;
    co_return;
}


int main()
{
#ifdef TEST_TIMER
    spdlog::set_level(spdlog::level::debug);
    galay::details::KqueueEventEngine::ptr engine = std::make_shared<galay::details::KqueueEventEngine>();
    GHandle handle{};
    galay::details::TimeEvent::CreateHandle(handle);
    
    galay::details::TimeEvent::ptr event = std::make_shared<galay::details::TimeEvent>(handle, engine.get());
    int i = 0;
    galay::Timer::ptr timer = event->AddTimer(1000, [event, &i](auto event, galay::Timer::ptr t) {
        std::cout << "timer expired" << std::endl;
        if(i++ < 10) event.lock()->AddTimer(1000, t);
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
    galay::InitializeGalayEnv({1, -1}, {0, -1}, {1, -1});
    test();
    std::cout << "main thread wait..." << std::endl;
    getchar();
    galay::DestroyGalayEnv();
    return 0;
}
#elif defined(TEST_DEADLINE)

galay::Coroutine::wptr p;

galay::Coroutine test()
{
    co_await galay::this_coroutine::GetThisCoroutine(p);
    co_await galay::this_coroutine::DeferExit([](){
        std::cout << "defer exit" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::cout << "defer exit end" << std::endl;
    });
    galay::this_coroutine::RoutineDeadLine deadline([](){
        std::cout << "deadline expired" << std::endl;
    });
    co_await deadline(100);
    for(int i = 0; i < 10; ++i) {
        deadline.Refluash();
    }
    std::cout << "after deadline" << std::endl;
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
    std::cout << "main thread wait..." << std::endl;
    galay::DestroyGalayEnv();

    return 0;
}

#elif defined(TEST_AUTO_DESTROY)

galay::Coroutine<void> test()
{
    //co_await galay::this_coroutine::AddToCoroutineStore();
    co_await galay::this_coroutine::DeferExit<void>([](){
        std::cout << "defer exit" << std::endl;
    });
    // galay::this_coroutine::AutoDestructor::ptr auto_destructor = std::make_shared<galay::this_coroutine::AutoDestructor>();
    // co_await auto_destructor->Start(1000);
    // std::cout << "sleep start" << std::endl;
    // co_await galay::this_coroutine::Sleepfor(10000);
    std::cout << "sleep end" << std::endl;
    co_return ;
}
 
int main()
{
    GALAY_APP_MAIN(
        test();
        std::cout << "main thread wait..." << std::endl;
        getchar();
    )
    return 0;
}



#endif