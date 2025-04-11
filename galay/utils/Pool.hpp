#ifndef GALAY_POOL_HPP
#define GALAY_POOL_HPP

#include <concurrentqueue/moodycamel/concurrentqueue.h>
#include <memory>
#include <cinttypes>
#include <concepts>

namespace galay::pool
{

template <typename T>
concept Object = requires(T t)
{
    { std::is_default_constructible_v<T> };
    { t.Reset() };
};

template<Object T>
class ProtocolPool
{
public:
    ProtocolPool(uint32_t capacity);
    std::unique_ptr<T> GetProtocol();
    void PutProtocol(std::unique_ptr<T> objector);
    ~ProtocolPool();
private:
    std::atomic_uint32_t m_capacity;
    moodycamel::ConcurrentQueue<std::unique_ptr<T>> m_queue;
};

template<typename T>
class SessionPool
{
public:
    SessionPool(uint32_t capacity);
    SessionPool(std::vector<T*> sessions);
    T* GetSession();
    void PutSession(T* session);
    ~SessionPool();
private:
    std::atomic_uint32_t m_capacity;
    moodycamel::ConcurrentQueue<T*> m_queue;
};


}

#include "Pool.tcc"

#endif