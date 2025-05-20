#ifndef GALAY_POOL_HPP
#define GALAY_POOL_HPP

#include <concurrentqueue/moodycamel/concurrentqueue.h>
#include <memory>
#include <cinttypes>
#include <concepts>
#include <functional>
#include <stdexcept>

namespace galay::utils
{

class Object
{
public:
    virtual void Reset() = 0;
    virtual ~Object() = default;
};

template <typename T>
concept IsObject = std::is_base_of_v<Object, T>;

template <IsObject T>
class ObjectPool
{
public:
    using CreateFunc = std::function<T*()>;
    using DestroyFunc = std::function<void(T*)>;

    ObjectPool(uint32_t size, 
               CreateFunc create_func = []{ return new T(); },
               DestroyFunc destroy_func = [](T* obj){ delete obj; });

    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

    ObjectPool(ObjectPool&& other) noexcept;
    ObjectPool& operator=(ObjectPool&& other) noexcept;

    ~ObjectPool();

    std::unique_ptr<T, std::function<void(T*)>> Acquire();

    bool Preallocate(uint32_t count);
    size_t Size() const;
    void Clear();

private:
    void Destroy(T* obj);

    CreateFunc m_createFunc;
    DestroyFunc m_destroyFunc;
    moodycamel::ConcurrentQueue<T*> m_queue;
};

} // namespace galay::utils

#include "Pool.tcc"

#endif // GALAY_POOL_HPP