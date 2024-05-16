#ifndef GALAY_COROUTINE_H
#define GALAY_COROUTINE_H

#include <coroutine>
#include <functional>
#include <memory>

namespace galay
{
    enum CoroutineStatus
    {
        GY_COROUTINE_NOT_EXIST,             // 协程不存在
        GY_COROUTINE_RUNNING,               // 协程运行
        GY_COROUTINE_SUSPEND,               // 协程挂起
        GY_COROUTINE_FINISHED,              // 协程结束

        GY_COROUTINE_WATTING_FOR_DATA,      //正在等待数据
    };

    template <typename RESULT>
    class GY_TcpPromise
    {
    public:
        using promise_type = GY_TcpPromise<RESULT>;
        inline static int get_return_object_on_alloaction_failure() noexcept;
        inline ::std::coroutine_handle<GY_TcpPromise> get_return_object();
        inline ::std::suspend_never initial_suspend() noexcept;
        inline ::std::suspend_always yield_value(const RESULT &value);
        inline ::std::suspend_always yield_value(RESULT value);
        inline ::std::suspend_always final_suspend() noexcept;
        inline void unhandled_exception() noexcept;
        inline void return_value(RESULT val) noexcept;
        // 获取结果
        inline RESULT GetResult();
        inline void SetResult(RESULT result);

        //获取状态
        inline CoroutineStatus GetStatus();
        inline void SetStatus(CoroutineStatus status);
        inline void SetFatherCoroutine(::std::coroutine_handle<promise_type> father_coroutine);
    private:
        inline void rethrow_if_exception();
    private:
        ::std::exception_ptr m_exception = nullptr;
        RESULT m_result;
        CoroutineStatus m_status = CoroutineStatus::GY_COROUTINE_RUNNING;
        ::std::coroutine_handle<> m_fatherhandle;
    };

    template <>
    class GY_TcpPromise<void>
    {
    public:
        inline static int get_return_object_on_alloaction_failure();
        inline ::std::coroutine_handle<GY_TcpPromise> get_return_object();
        inline ::std::suspend_never initial_suspend() noexcept;
        template <typename T>
        inline ::std::suspend_always yield_value(const T &value);
        inline ::std::suspend_always final_suspend() noexcept;
        inline void unhandled_exception() noexcept;
        inline void return_void() noexcept {}
        //获取状态
        inline CoroutineStatus GetStatus();
        inline void SetStatus(CoroutineStatus status);
        inline void SetFatherCoroutine(::std::coroutine_handle<> father_coroutine);
    private:
        inline void rethrow_if_exception();
    private:
        ::std::exception_ptr m_exception = {};
        CoroutineStatus m_status = CoroutineStatus::GY_COROUTINE_RUNNING;
        ::std::coroutine_handle<> m_fathercoroutine;
    };

    template <typename RESULT>
    class GY_TcpPromiseSuspend
    {
    public:
        using promise_type = GY_TcpPromiseSuspend<RESULT>;
        inline static int get_return_object_on_alloaction_failure() noexcept;
        inline ::std::coroutine_handle<GY_TcpPromiseSuspend> get_return_object();
        inline ::std::suspend_always initial_suspend() noexcept;
        inline ::std::suspend_always yield_value(const RESULT &value);
        inline ::std::suspend_always yield_value(RESULT value);
        inline ::std::suspend_always final_suspend() noexcept;
        inline void unhandled_exception() noexcept;
        inline void return_value(RESULT val) noexcept;
        // 获取结果
        inline RESULT GetResult();
        inline void SetResult(RESULT result);

        //获取状态
        inline CoroutineStatus GetStatus();
        inline void SetStatus(CoroutineStatus status);
        inline void SetFatherCoroutine(::std::coroutine_handle<promise_type> father_coroutine);
    private:
        inline void rethrow_if_exception();
    private:
        ::std::exception_ptr m_exception = nullptr;
        RESULT m_result;
        CoroutineStatus m_status = CoroutineStatus::GY_COROUTINE_SUSPEND;
        ::std::coroutine_handle<> m_fatherhandle;
    };

    template <>
    class GY_TcpPromiseSuspend<void>
    {
    public:
        inline static int get_return_object_on_alloaction_failure();
        inline ::std::coroutine_handle<GY_TcpPromiseSuspend> get_return_object();
        inline ::std::suspend_always initial_suspend() noexcept;
        template <typename T>
        inline ::std::suspend_always yield_value(const T &value);
        inline ::std::suspend_always final_suspend() noexcept;
        inline void unhandled_exception() noexcept;
        inline void return_void() noexcept {}
        //获取状态
        inline CoroutineStatus GetStatus();
        inline void SetStatus(CoroutineStatus status);
        inline void SetFatherCoroutine(::std::coroutine_handle<> father_coroutine);
    private:
        inline void rethrow_if_exception();
    private:
        ::std::exception_ptr m_exception = {};
        CoroutineStatus m_status = CoroutineStatus::GY_COROUTINE_RUNNING;
        ::std::coroutine_handle<> m_fathercoroutine;
    };


    template <typename RESULT>
    class GY_Coroutine
    {
    public:
        using promise_type = GY_TcpPromise<RESULT>;
        GY_Coroutine(const GY_Coroutine &other) = delete;
        GY_Coroutine &operator=(GY_Coroutine &other) = delete;
        GY_Coroutine<RESULT> &operator=(const GY_Coroutine<RESULT> &other) = delete;

        GY_Coroutine<RESULT> &operator=(GY_Coroutine<RESULT> &&other);
        GY_Coroutine() = default;
        GY_Coroutine(::std::coroutine_handle<promise_type> co_handle) noexcept;
        GY_Coroutine(GY_Coroutine<RESULT> &&other) noexcept;

        void Resume() noexcept;
        bool Done() noexcept;
        ~GY_Coroutine();
    protected:
        // 协程句柄
        ::std::coroutine_handle<promise_type> m_handle = nullptr;
    };

    template <typename RESULT = CoroutineStatus>
    class GY_TcpCoroutine : public GY_Coroutine<RESULT>
    {
    public:
        using promise_type = GY_TcpPromise<RESULT>;
        using ptr = std::shared_ptr<GY_TcpCoroutine>;
        using uptr = std::unique_ptr<GY_TcpCoroutine>;
        using wptr = std::weak_ptr<GY_TcpCoroutine>;

        GY_TcpCoroutine() = default;
        GY_TcpCoroutine(::std::coroutine_handle<promise_type> co_handle) noexcept;
        GY_TcpCoroutine(GY_TcpCoroutine<RESULT> &&other) noexcept;
        GY_TcpCoroutine<RESULT> &operator=(const GY_TcpCoroutine<RESULT> &other) = delete;
        GY_TcpCoroutine<RESULT> &operator=(GY_TcpCoroutine<RESULT> &&other);  
        std::coroutine_handle<promise_type> GetCoroutine() const;
        void SetFatherCoroutine(const GY_TcpCoroutine<RESULT>& father_coroutine);
        //是否是协程
        bool IsCoroutine();
        //获取结果
        RESULT GetResult();
        void SetResult(RESULT result);
        //获取状态
        CoroutineStatus GetStatus();
        void SetStatus(CoroutineStatus status);
        ~GY_TcpCoroutine();
    };

    //not to use
    template <typename RESULT>
    class GY_CoroutineSuspend
    {
    public:
        using promise_type = GY_TcpPromiseSuspend<RESULT>;
        GY_CoroutineSuspend(const GY_CoroutineSuspend &other) = delete;
        GY_CoroutineSuspend &operator=(GY_CoroutineSuspend &other) = delete;
        GY_CoroutineSuspend<RESULT> &operator=(const GY_CoroutineSuspend<RESULT> &other) = delete;

        GY_CoroutineSuspend<RESULT> &operator=(GY_CoroutineSuspend<RESULT> &&other);
        GY_CoroutineSuspend() = default;
        GY_CoroutineSuspend(::std::coroutine_handle<promise_type> co_handle) noexcept;
        GY_CoroutineSuspend(GY_CoroutineSuspend<RESULT> &&other) noexcept;
        void Resume() noexcept;
        bool Done() noexcept;
        ~GY_CoroutineSuspend();
    protected:
        ::std::coroutine_handle<promise_type> m_handle;
    };

    template <typename RESULT = CoroutineStatus>
    class GY_TcpCoroutineSuspend : public GY_CoroutineSuspend<RESULT>
    {
    public:
        using promise_type = GY_TcpPromiseSuspend<RESULT>;
        using ptr = std::shared_ptr<GY_TcpCoroutineSuspend>;
        using uptr = std::unique_ptr<GY_TcpCoroutineSuspend>;
        using wptr = std::weak_ptr<GY_TcpCoroutineSuspend>;

        GY_TcpCoroutineSuspend<RESULT> &operator=(const GY_TcpCoroutineSuspend<RESULT> &other) = delete;

        GY_TcpCoroutineSuspend() = default;
        GY_TcpCoroutineSuspend(::std::coroutine_handle<promise_type> co_handle) noexcept;
        GY_TcpCoroutineSuspend(GY_TcpCoroutineSuspend<RESULT> &&other) noexcept;
        GY_TcpCoroutineSuspend<RESULT> &operator=(GY_TcpCoroutineSuspend<RESULT> &&other);  
        std::coroutine_handle<promise_type> GetCoroutine() const;
        void SetFatherCoroutine(const GY_TcpCoroutineSuspend<RESULT>& father_coroutine);
        //是否是协程
        bool IsCoroutine();
        //获取结果
        RESULT GetResult();
        void SetResult(RESULT result);
        //获取状态
        CoroutineStatus GetStatus();
        void SetStatus(CoroutineStatus status);
        ~GY_TcpCoroutineSuspend();
    };

    #include "coroutine.inl"
}

#endif