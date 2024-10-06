#include "../galay/galay.h"
#include <thread>
#include <vector>
#include <iostream>
#include <sys/timerfd.h>
#include <spdlog/spdlog.h>

#define EVENT_SCHEDULER_NUM 4

#define TEST_NET_EVENT

#ifdef TEST_STOP_EVENT 
void Test()
{
    std::vector<std::thread> threads;
    std::vector<EventScheduler> schedulers(EVENT_SCHEDULER_NUM);
    ThreadWaiters::ptr waiter = std::make_shared<ThreadWaiters>(EVENT_SCHEDULER_NUM);
    for(int i = 0 ; i < EVENT_SCHEDULER_NUM ; ++i)
    {
        threads.push_back(std::thread([i, &schedulers, waiter](){
            if(!schedulers[i].Loop()) {
                std::cout << schedulers[i].GetLastError() << std::endl;
            }
            std::cout << "thread " << i << " exit" << std::endl;
            waiter->Decrease();
        }));
        threads[i].detach();
    }
    for(int i = 0 ; i < EVENT_SCHEDULER_NUM ; ++i) 
    {
        while(!schedulers[i].Stop()) {
            std::this_thread::yield();
            std::cout << "stop scheduler " << i << std::endl;
        }
    }
    if(!waiter->Wait(1000)) {
        std::cout << "wait timeout" << std::endl;
    };
    sleep(1);
}
#elif defined(TEST_COROUTINE_EVENT)
galay::coroutine::Coroutine Func(galay::scheduler::CoroutineScheduler::ptr scheduler)
{
    std::cout << "ready to wait\n";
    co_await std::suspend_always{};
    std::cout << "wait finish" << std::endl;
    co_return;
}

void Test()
{
    galay::scheduler::ResizeCoroutineSchedulers(1);
    galay::scheduler::StartCoroutineSchedulers();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    auto Work = galay::scheduler::GetCoroutineScheduler(0);
    auto co = Func(Work);
    Work->AddCoroutine(co.GetHandle().promise().GetCoroutine());
    getchar();
    galay::scheduler::StopCoroutineSchedulers();
}

#elif defined(TEST_NET_EVENT)
void Test()
{
    galay::scheduler::ResizeCoroutineSchedulers(1);
    galay::scheduler::StartCoroutineSchedulers();
    galay::scheduler::ResizeNetIOSchedulers(EVENT_SCHEDULER_NUM);
    galay::scheduler::StartNetIOSchedulers();
    GHandle handle = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(7070);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(handle,(sockaddr*)&addr, sizeof(addr));
    listen(handle, 5);
    setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, (char*)&addr, sizeof(addr));
    int option = 1;
    setsockopt(handle, SOL_SOCKET, SO_REUSEPORT, &option, sizeof(option));
    galay::event::UnaryStrategy::ptr strategy = std::make_shared<galay::event::UnaryStrategy>([](galay::event::UnaryOperation* operation, galay::coroutine::CoroutineWaitContext* context) ->galay::coroutine::Coroutine{
        std::string_view data = operation->GetReadBuffer();
        std::cout << "recv data " << data << std::endl;
        operation->SetWriteBuffer(data, true);
        co_return;
    });
    
    for( int i = 0 ; i < EVENT_SCHEDULER_NUM ; ++i )
    {
        galay::event::ListenEvent* listen_event = new galay::event::ListenEvent(handle, strategy, \
            galay::scheduler::GetNetIOScheduler(i), galay::scheduler::GetCoroutineScheduler(0), true);
        galay::scheduler::GetNetIOScheduler(i)->AddEvent(listen_event);
    }
    getchar();
    galay::scheduler::StopCoroutineSchedulers();
    galay::scheduler::StopNetIOSchedulers();
}
#elif defined(TEST_COROUTINE_WAITERS)

galay::coroutine::Coroutine func()
{   
    galay::coroutine::CoroutineWaiters waiters(2, 0);
    std::thread th([&waiters](){
        std::cout << "thread ready" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        waiters.Decrease();
        std::cout << "thread ready" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        waiters.Decrease();
        std::cout << "thread exit" << std::endl;
    });
    th.detach();
    bool ret = co_await waiters.Wait();
    std::cout << "wait finish " << ret << std::endl;
}


void Test()
{
    galay::scheduler::ResizeCoroutineSchedulers(1);
    galay::scheduler::StartCoroutineSchedulers();
    func();
    getchar();
    galay::scheduler::StopCoroutineSchedulers();
}

#elif defined(TEST_TIMER_EVENT)

void Test()
{
    galay::scheduler::ResizeNetIOSchedulers(1);
    galay::scheduler::StartNetIOSchedulers();
    GHandle timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
    galay::event::TimeEvent* time_event = new galay::event::TimeEvent(timerfd);
    galay::scheduler::GetNetIOScheduler(0)->AddEvent(time_event);
    time_event->AddTimer(1000, [time_event](galay::event::TimeEvent::Timer::ptr timer){
        std::cout << "timer callback" << std::endl;
        std::any& tmp = timer->GetContext();
        if(! tmp.has_value() ) {
            tmp = 1;
        }  
        int t = std::any_cast<int>(tmp);
        if( t <= 10 ){
            tmp = ++t;
            time_event->ReAddTimer(1000, timer);
        }
        
    });
    getchar();
    galay::scheduler::StopNetIOSchedulers();
}
    
    
#endif

int main()
{
    spdlog::set_level(spdlog::level::debug);
    Test();
    return 0;
}