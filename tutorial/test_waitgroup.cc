#include "galay/galay.hpp"
#include <iostream>
using galay::CoroutineBase;
using galay::WaitGroup;

#define TEST_ROUTINE_CONTEXT_WITH_WAIT_GROUP

#ifdef TEST_WAITGROUP

Coroutine pfunc()
{
    WaitGroup wg(4);
    std::cout << "pfunc start" << std::endl;
    for(int i = 0; i < 4; i++) {
        std::thread t([&wg]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            wg.Done();
            std::cout << "wg.Done()" << std::endl;
        });
        t.detach();
    }
    co_await wg.Wait();
    std::cout << "pfunc end" << std::endl;
}

int main()
{
    galay::InitializeGalayEnv(0, 1, 0, -1);
    pfunc();
    getchar();
    galay::DestroyGalayEnv();
    return 0;
}

#elif defined(TEST_ROUTINE_CONTEXT)
Coroutine::wptr p = nullptr;

Coroutine cfunc(RoutineContext::ptr context)
{
    co_await context->DeferDone();
    co_await galay::this_coroutine::DeferExit([]{
        std::cout << "cfunc exit" << std::endl;
    });
    co_return;
}

Coroutine pfunc()
{
    auto [context, cancle] = galay::ContextFactory::WithNewContext();
    co_await galay::this_coroutine::GetThisCoroutine(p);
    std::cout << "pfunc start" << std::endl;
    cfunc(context);
    co_await galay::this_coroutine::DeferExit([cancle]{
        std::cout << "pfunc exit" << std::endl;
        (*cancle)();
    });
    co_await std::suspend_always{};
    co_return;
}

int main()
{
    galay::InitializeGalayEnv(0, 1, 0, -1);
    pfunc();
    getchar();
    p->Destroy();
    getchar();
    galay::DestroyGalayEnv();
    return 0;
}

#elif defined(TEST_ROUTINE_CONTEXT_WITH_WAIT_GROUP)

CoroutineBase::wptr p{};


int main()
{
    galay::GalayEnvConf conf;
    galay::GalayEnv env(conf);
    getchar();
    p.lock()->Destroy();
    getchar();
    return 0;
}

#endif