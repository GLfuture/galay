// #include <iostream>
// #include <string>
// #include <regex>
// #include <iostream>
// #include <galay/galay.hpp>

// using namespace galay::mysql;
// using namespace galay;


// Coroutine<void> test()
// {
//     co_return;
// }

// int main()
// {
//     InitializeGalayEnv({1, -1}, {1, -1}, {1, -1});
//     auto func = []() -> galay::Coroutine<int> {
//         co_await galay::this_coroutine::DeferExit<int>([](){
//             std::cout << "exit\n";
//         });
//         co_await galay::this_coroutine::Sleepfor<int>(1000);
//         std::cout << "sleep 1s" << std::endl;
//         co_return 1;
//     };
//     auto coroutine = func();
//     std::cout << sizeof(galay::http::Http2FrameHeader) << std::endl;
//     getchar();
//     std::cout << coroutine().value() << std::endl;
//     DestroyGalayEnv();
//     return 0;
// }

#include "galay/galay.hpp"
#include <iostream>

// template<typename CoRtn, typename ...Args>
// struct std::coroutine_traits<galay::Coroutine<CoRtn>, Args...> {
//     using promise_type = galay::Coroutine<CoRtn>::promise_type;
//     static promise_type get_promise(galay::details::CoroutineScheduler* scheduler) noexcept {
//         return promise_type(scheduler);
//     }
// };

#define TEST_FATHER_EXIT

#ifdef TEST_FATHER_EXIT

galay::Coroutine<int> func(galay::RoutineCtx ctx)
{
    //auto timer = ctx->WithTimeout(3000, [](){ std::cout << "timeout" << std::endl; });
    co_await galay::this_coroutine::Sleepfor<int>(3000);
    std::cout << "sleep 2s" << std::endl;
    auto& graph = ctx.GetSharedCtx().lock()->GetRoutineGraph();
    std::cout << "func\n";
    for(int i = 0; i < graph.size(); ++i) {
        std::cout << "layer: " << i << std::endl;
        for(auto& coroutine: graph[i]) {
            std::cout << "sequence: " << coroutine.first << " " << std::endl;
        }
    }
    co_return 1;
}


class A 
{
public:
    galay::Coroutine<int> func(galay::RoutineCtx ctx) {
        return ::func(ctx.Copy());
    }
};


galay::Coroutine<int> test(galay::RoutineCtx ctx)
{
    int p = 10;
    A a;
    // auto co = co_await galay::this_coroutine::WaitAsyncExecute<int, int>(&A::func, &a, ctx);
    // std::cout << "end" << std::endl;
    // auto res = (*co)();
    // if(res.has_value()) {
    //     co_return std::move(res.value());
    // }
    std::cout << "TEST BEGIN........\n";
    a.func(ctx.Copy());
    a.func(ctx.Copy());

    std::cout << "test: layer: " << ctx.GetThisLayer() << '\n';
    auto& graph = ctx.GetSharedCtx().lock()->GetRoutineGraph();
    for(int i = 0; i < graph.size(); ++i) {
        std::cout << "layer: " << i << std::endl;
        for(auto& coroutine: graph[i]) {
            std::cout << "sequence: " << coroutine.first << " " << std::endl;
        }
    }
    for(int i = ctx.GetThisLayer() + 1; i < graph.size(); ++i) {
        std::cout << "layer: " << i << std::endl;
        for(auto& coroutine: graph[i]) {
            std::cout << "sequence: " << coroutine.first << " " << std::endl;
            if(!coroutine.second.expired()) coroutine.second.lock()->BelongScheduler()->ToDestroyCoroutine(coroutine.second);
        }
    }
    
    co_return 0;
}


#else
galay::Coroutine<int> func(galay::RoutineCtx ctx)
{
    auto timer = ctx->WithTimeout(1000, [](){ std::cout << "timeout" << std::endl; });
    co_await galay::this_coroutine::Sleepfor<int>(2000);
    std::cout << "sleep 2s" << std::endl;
    timer->Cancle();
    co_return 1;
}


class A 
{
public:
    galay::Coroutine<int> func(galay::RoutineCtx ctx) {
        return ::func(ctx);
    }
};


galay::Coroutine<int> test(galay::RoutineCtx ctx)
{
    int p = 10;
    A a;
    auto co = co_await galay::this_coroutine::WaitAsyncExecute<int, int>(&A::func, &a, ctx);
    std::cout << "end" << std::endl;
    auto res = (*co)();
    if(res.has_value()) {
        co_return std::move(res.value());
    }
    co_return 0;
}

#endif


int main()
{
    galay::GalayEnv env({{1, -1}, {1, -1}, {1, -1}});
    std::cout << "BEGIN.....\n";
    auto co = test(galay::RoutineCtx::Create());
    std::cout << "start" << std::endl;
    auto op = co();
    getchar();
    if(op.has_value()) std::cout << "getchar " << op.value() << std::endl;
    return 0;
}