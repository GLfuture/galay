#include "galay/galay.hpp"
#include <iostream>
using galay::CoroutineBase;
using galay::WaitGroup;

#define TEST_WAIT_BLOCK_EXECUTE

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

#elif defined(TEST_WAIT_BLOCK_EXECUTE)

using namespace galay;

Coroutine<int> do_things(RoutineCtx ctx, int x)
{
    co_await this_coroutine::DeferExit<int>([](auto co) {
        std::cout << "do_things exit" << std::endl;
    });
    co_await this_coroutine::Sleepfor<int>(1000);
    std::cout << "sleep over\n";
    co_return 1;
}

CoroutineBase::wptr p;

Coroutine<void> test(RoutineCtx ctx)
{
    co_await this_coroutine::DeferExit<void>([](auto co) {
        std::cout << "test exit" << std::endl;
    });
    p = co_await this_coroutine::GetThisCoroutine<void>();
    int a = 0;
    auto res = co_await this_coroutine::WaitAsyncRtnExecute<int, void>(do_things, ctx, a);
    std::cout << "res = " << res << std::endl;
    co_return;
}

int main()
{
    galay::GalayEnvConf conf;
    galay::GalayEnv env(conf);
    test(RoutineCtx::Create(galay::EventSchedulerHolder::GetInstance()->GetScheduler(0)));
    p.lock()->GetCoScheduler()->ToDestroyCoroutine(p);
    getchar();
    return 0;
}
#endif