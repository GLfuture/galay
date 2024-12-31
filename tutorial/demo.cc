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

galay::Coroutine<int> func(galay::RoutineCtx ct)
{
    co_await galay::this_coroutine::Sleepfor<int>(2000);
    std::cout << "sleep 2s" << std::endl;
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
    auto co = co_await galay::this_coroutine::WaitAsyncExecute<int, int>(&A::func, &a);
    std::cout << "end" << std::endl;
    co_return (*co)().value();
}




int main()
{
    galay::InitializeGalayEnv({1, -1}, {1, -1}, {1, -1});
    auto co = test({});
    getchar();
    std::cout << "getchar " << co().value() << std::endl;
    galay::DestroyGalayEnv();
    return 0;
}