#include "galay/galay.hpp"
#include "galay/kernel/EventEngine.h"
#include <iostream>

#define TEST_ASYNC_TIMEOUT

#ifdef TEST_SLEEP
galay::Coroutine<void> test(galay::RoutineCtx ctx)
{
    std::cout << "start" << std::endl;
    co_await galay::this_coroutine::Sleepfor<void>(5000);
    std::cout << "sleep 5s" << std::endl;
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
    galay::GalayEnvConf conf;
    conf.m_coroutineSchedulerConf.m_thread_num = 1;
    conf.m_eventSchedulerConf.m_thread_num = 1;
    conf.m_eventSchedulerConf.m_time_manager_type = galay::EventSchedulerTimerManagerType::kEventSchedulerTimerManagerTypeRbTree;
    galay::GalayEnv env(conf);
    test(galay::RoutineCtx::Create(galay::EventSchedulerHolder::GetInstance()->GetScheduler(0)));
    std::cout << "main thread wait..." << std::endl;
    getchar();
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

galay::Coroutine<void> test(galay::RoutineCtx ctx)
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
    galay::GalayEnv env({{1, -1}, {1, -1}, {1, -1}});
    test(galay::RoutineCtx::Create());
    std::cout << "main thread wait..." << std::endl;
    getchar();
    
    return 0;
}

#elif defined(TEST_TIMER)

int i = 0;

void func(galay::details::TimeEvent::wptr event, galay::Timer::ptr timer)
{
    std::cout << "timer expired " << i++ << std::endl;
}

int main()
{
    galay::details::EventScheduler scheduler;
    scheduler.Loop(-1);
    galay::Timer::ptr timer1 = galay::Timer::Create(func);
    //int64_t timeout, std::function<void(std::weak_ptr<details::TimeEvent>, Timer::ptr)> &&func
    scheduler.AddTimer(timer1, 2000);
    galay::Timer::ptr timer2 = galay::Timer::Create(func);
    scheduler.AddTimer(timer2, 500);
    getchar();
    scheduler.Stop();
    return 0;
}

#elif defined(TEST_ASYNC_TIMEOUT)

galay::Coroutine<void> test(galay::RoutineCtx ctx)
{
    galay::AsyncTcpSocket socket;
    socket.Socket();
    socket.Bind("127.0.0.1", 8090);
    socket.Listen(10);
    while(true) {
        galay::THost host;
        GHandle handle = co_await socket.Accept<void>(&host, 10000);
        std::cout << "accept :" << host.m_ip << ": " << host.m_port << std::endl;
        std::cout << "handle :" << handle.fd << std::endl << "------------------------\n";
        galay::TcpIOVecHolder vec;
        int byte = co_await socket.Recv<void>(vec, 512, 10000);
        std::cout << "recv :" << byte << " : " << std::string(vec->m_buffer, byte) << std::endl;
        co_return;
    };
    
}

int main()
{
    galay::GalayEnvConf conf;
    conf.m_coroutineSchedulerConf.m_thread_num = 1;
    conf.m_eventSchedulerConf.m_thread_num = 1;
    galay::GalayEnv env(conf);
    test(galay::RoutineCtx::Create(galay::EventSchedulerHolder::GetInstance()->GetScheduler(0)));
    getchar();
    return 0;
}

#endif