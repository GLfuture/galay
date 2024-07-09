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
#include <spdlog/spdlog.h>

namespace galay
{
    namespace common
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

        template <typename RESULT>
        class GY_TcpPromise
        {
        public:
            using promise_type = GY_TcpPromise<RESULT>;
            inline static int get_return_object_on_alloaction_failure() noexcept;
            inline std::coroutine_handle<GY_TcpPromise> get_return_object();
            inline std::suspend_never initial_suspend() noexcept;
            inline std::suspend_always yield_value(const RESULT &value);
            inline std::suspend_always yield_value(RESULT value);
            inline std::suspend_always final_suspend() noexcept;
            inline void unhandled_exception() noexcept;
            inline void return_value(RESULT val) noexcept;
            // 获取结果
            inline RESULT GetResult();
            inline void SetResult(RESULT result);

            // 获取状态
            inline CoroutineStatus GetStatus();
            inline void SetStatus(CoroutineStatus status);
            inline void SetFinishFunc(std::function<void()> finshfunc);
        private:
            inline void rethrow_if_exception();

        private:
            std::exception_ptr m_exception = nullptr;
            RESULT m_result;
            CoroutineStatus m_status = CoroutineStatus::kCoroutineNotExist;
            std::function<void()> m_finishFunc;
        };

        template <>
        class GY_TcpPromise<void>
        {
        public:
            inline static int get_return_object_on_alloaction_failure();
            inline std::coroutine_handle<GY_TcpPromise> get_return_object();
            inline std::suspend_never initial_suspend() noexcept;
            template <typename T>
            inline std::suspend_always yield_value(const T &value);
            inline std::suspend_always final_suspend() noexcept;
            inline void unhandled_exception() noexcept;
            inline void return_void() noexcept {}
            // 获取状态
            inline CoroutineStatus GetStatus();
            inline void SetStatus(CoroutineStatus status);
            inline void SetFinishFunc(std::function<void()> finshfunc);
        private:
            inline void rethrow_if_exception();

        private:
            std::exception_ptr m_exception = {};
            CoroutineStatus m_status = CoroutineStatus::kCoroutineNotExist;
            std::function<void()> m_finishFunc;
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
            GY_Coroutine(std::coroutine_handle<promise_type> co_handle) noexcept;
            GY_Coroutine(GY_Coroutine<RESULT> &&other) noexcept;

            void Resume() noexcept;
            bool Done() noexcept;
            uint64_t GetCoId() const noexcept;

            ~GY_Coroutine();

        protected:
            // 协程句柄
            std::coroutine_handle<promise_type> m_handle = nullptr;
        };

        template <typename RESULT = CoroutineStatus>
        class GY_NetCoroutine : public GY_Coroutine<RESULT>
        {
        public:
            using promise_type = GY_TcpPromise<RESULT>;
            using ptr = std::shared_ptr<GY_NetCoroutine>;
            using uptr = std::unique_ptr<GY_NetCoroutine>;
            using wptr = std::weak_ptr<GY_NetCoroutine>;

            GY_NetCoroutine() = default;
            GY_NetCoroutine(std::coroutine_handle<promise_type> co_handle) noexcept;
            GY_NetCoroutine(GY_NetCoroutine<RESULT> &&other) noexcept;
            GY_NetCoroutine<RESULT> &operator=(const GY_NetCoroutine<RESULT> &other) = delete;
            GY_NetCoroutine<RESULT> &operator=(GY_NetCoroutine<RESULT> &&other);
            // 是否是协程
            bool IsCoroutine();
            // 获取结果
            RESULT GetResult();
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
            GY_NetCoroutinePool(std::shared_ptr<std::condition_variable> fcond);
            void Start();
            void Stop();
            bool Contains(uint64_t coId);
            bool Resume(uint64_t coId, bool always);
            bool AddCoroutine(uint64_t coId, GY_NetCoroutine<CoroutineStatus>&& coroutine);
            bool EraseCoroutine(uint64_t coId);
            GY_NetCoroutine<CoroutineStatus>& GetCoroutine(std::pair<uint64_t,bool> id_always);
            virtual ~GY_NetCoroutinePool();
        private:
            void RealEraseCoroutine(uint64_t coId);
        private:
            //pair<coid,exec times>
            std::queue<std::pair<uint64_t,bool>> m_waitCoQueue;
            std::map<uint64_t,GY_NetCoroutine<CoroutineStatus>> m_coroutines;
            std::queue<uint64_t> m_eraseCoroutines;
            std::atomic_bool m_stop;
            std::mutex m_queueMtx;
            std::shared_mutex m_mapMtx;
            std::condition_variable m_cond;
            std::shared_ptr<std::condition_variable> m_fcond;
        };

#include "coroutine.inl"
    }
}

#endif
