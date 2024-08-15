#ifndef GALAY_COROUTINE_H
#define GALAY_COROUTINE_H

#include <coroutine>
#include <functional>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <map>
#include <queue>
#include <any>
#include <set>
#include "base.h"
#include <spdlog/spdlog.h>

namespace galay
{
    namespace coroutine
    {
        enum CoroutineStatus
        {
            kCoroutineNotExist,       // 协程不存在
            kCoroutineRunning,        // 协程运行
            kCoroutineSuspend,        // 协程挂起
            kCoroutineFinished,       // 协程结束
            kCoroutineWaitingForData, // 正在等待数据
        };
        class GY_NetCoroutinePool;
        class GY_Coroutine;
        class GY_NetCoroutine;

        class GY_TcpPromise
        {
        public:
            using promise_type = GY_TcpPromise;
            static int get_return_object_on_alloaction_failure() noexcept;
            GY_NetCoroutine get_return_object();
            std::suspend_never initial_suspend() noexcept;
            std::suspend_always yield_value(std::any value);
            std::suspend_never final_suspend() noexcept;
            void unhandled_exception() noexcept;
            void return_value(std::any val) noexcept;
            // 获取结果
            std::any GetResult();
            void SetResult(std::any result);

            // 获取状态
            CoroutineStatus GetStatus();
            void SetStatus(CoroutineStatus status);
            void SetFinishFunc(std::function<void()> finshfunc);
        private:
            void rethrow_if_exception();

        private:
            std::exception_ptr m_exception = nullptr;
            std::any m_result;
            uint64_t m_coId;
            CoroutineStatus m_status = CoroutineStatus::kCoroutineNotExist;
            std::function<void()> m_finishFunc;
        };

        // template <>
        // class GY_TcpPromise<void>
        // {
        // public:
        //     static int get_return_object_on_alloaction_failure();
        //     std::coroutine_handle<GY_TcpPromise> get_return_object();
        //     std::suspend_never initial_suspend() noexcept;
        //     template <typename T>
        //     std::suspend_always yield_value(const T &value);
        //     std::suspend_never final_suspend() noexcept;
        //     void unhandled_exception() noexcept;
        //     void return_void() noexcept {}
        //     // 获取状态
        //     CoroutineStatus GetStatus();
        //     void SetStatus(CoroutineStatus status);
        //     void SetFinishFunc(std::function<void()> finshfunc);
        // private:
        //     void rethrow_if_exception();

        // private:
        //     std::exception_ptr m_exception = {};
        //     CoroutineStatus m_status = CoroutineStatus::kCoroutineNotExist;
        //     std::function<void()> m_finishFunc;
        // };

        
        class GY_Coroutine
        {
        public:
            using promise_type = GY_TcpPromise;
            GY_Coroutine() = default;
            GY_Coroutine &operator=(GY_Coroutine &other) = delete;
            GY_Coroutine &operator=(const GY_Coroutine &other) = delete;
            GY_Coroutine(const GY_Coroutine &other) = delete;
            GY_Coroutine &operator=(GY_Coroutine &&other);
            GY_Coroutine(GY_Coroutine &&other) noexcept;
            GY_Coroutine(std::coroutine_handle<promise_type> co_handle) noexcept;
            void Resume() noexcept;
            bool Done() noexcept;
            uint64_t GetCoId() const noexcept;

            ~GY_Coroutine();

        protected:
            // 协程句柄
            std::coroutine_handle<promise_type> m_handle = nullptr;
        };

        class GY_NetCoroutine: public GY_Coroutine, std::enable_shared_from_this<GY_NetCoroutine>
        {
        public:
            using promise_type = GY_TcpPromise;
            using ptr = std::shared_ptr<GY_NetCoroutine>;
            using uptr = std::unique_ptr<GY_NetCoroutine>;
            using wptr = std::weak_ptr<GY_NetCoroutine>;

            GY_NetCoroutine() = default;
            GY_NetCoroutine(std::coroutine_handle<promise_type> co_handle) noexcept;
            GY_NetCoroutine(GY_NetCoroutine &&other) noexcept;
            GY_NetCoroutine &operator=(GY_NetCoroutine &&other) noexcept;
            GY_NetCoroutine &operator=(const GY_NetCoroutine &other) = delete;
            GY_NetCoroutine(const GY_NetCoroutine &other) = delete;
            // 是否是协程
            bool IsCoroutine();
            // 获取结果
            std::any GetResult();
            // 获取状态
            CoroutineStatus GetStatus();
            ~GY_NetCoroutine();
        };
        

        class GY_NetCoroutinePool
        {
        public:
            using ptr = std::shared_ptr<GY_NetCoroutinePool>;
            using wptr = std::weak_ptr<GY_NetCoroutinePool>;
            using uptr = std::unique_ptr<GY_NetCoroutinePool>;
            static void SetThreadNum(uint16_t threadNum);
            static GY_NetCoroutinePool* GetInstance();
            GY_NetCoroutinePool();
            void Start();
            void Stop();
            bool IsDone();
            bool WaitForAllDone(uint32_t timeout = 0); //ms
            bool Contains(uint64_t coId);
            bool Resume(uint64_t coId);
            bool AddCoroutine(GY_NetCoroutine::ptr coroutine);
            bool EraseCoroutine(uint64_t coId);
            GY_NetCoroutine::ptr GetCoroutine(uint64_t id);
            virtual ~GY_NetCoroutinePool();
        private:
            void RealEraseCoroutine(uint64_t coId);
            void Run();
        private:
            static uint16_t m_threadNum;
            static std::unique_ptr<GY_NetCoroutinePool> m_instance;
            static std::atomic_bool m_isStarted;
            std::atomic_bool m_isDone;
            std::atomic_bool m_stop;
            std::atomic_bool m_isStopped;
            std::mutex m_queueMtx;
            std::shared_mutex m_mapMtx;
            std::shared_mutex m_eraseMtx;
            std::condition_variable m_cond;
            std::condition_variable m_exitCond;
            std::queue<uint64_t> m_waitCoQueue;
            std::set<uint64_t> m_eraseCoroutines;
            std::vector<std::unique_ptr<std::thread>> m_threads;
            std::map<uint64_t,GY_NetCoroutine::ptr> m_coroutines;
            
        };
    }
}

#endif
