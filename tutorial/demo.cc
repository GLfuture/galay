// #include <iostream>
// #include <string>
// #include <regex>
// #include <iostream>
// #include <galay/galay.h>

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
//     std::cout << sizeof(galay::protocol::http::Http2FrameHeader) << std::endl;
//     getchar();
//     std::cout << coroutine().value() << std::endl;
//     DestroyGalayEnv();
//     return 0;
// }

#include "galay/galay.h"
#include <iostream>

// template<typename CoRtn, typename ...Args>
// struct std::coroutine_traits<galay::Coroutine<CoRtn>, Args...> {
//     using promise_type = galay::Coroutine<CoRtn>::promise_type;
//     static promise_type get_promise(galay::details::CoroutineScheduler* scheduler) noexcept {
//         return promise_type(scheduler);
//     }
// };


class A 
{
public:
    galay::Coroutine<void> func(galay::RoutineCtx ct)
    {
        co_await galay::this_coroutine::Sleepfor<void>(2000);
        std::cout << "sleep 2s" << std::endl;
        co_return;
    }
};




galay::Coroutine<void> test()
{
    int p = 10;
    A a;
    co_await galay::this_coroutine::WaitAsyncExecute(&A::func, &a);
    std::cout << "end" << std::endl;
    co_return;
}




int main()
{
    galay::InitializeGalayEnv({1, -1}, {1, -1}, {1, -1});
    auto co = test();
    std::cout << "start" << std::endl;
    getchar();
    galay::DestroyGalayEnv();
    return 0;
}