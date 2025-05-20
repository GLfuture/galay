#ifndef GALAY_POOL_TCC
#define GALAY_POOL_TCC

#include "Pool.hpp"

namespace galay::utils
{

template <IsObject T>
ObjectPool<T>::ObjectPool(uint32_t size, CreateFunc create_func, DestroyFunc destroy_func)
    : m_createFunc(std::move(create_func)),
      m_destroyFunc(std::move(destroy_func)),
      m_queue(size)
{
    for (uint32_t i = 0; i < size; ++i) {
        T* obj = m_createFunc();
        m_queue.enqueue(obj);
    }
}

template <IsObject T>
ObjectPool<T>::ObjectPool(ObjectPool&& other) noexcept
    : m_createFunc(std::move(other.m_createFunc)),
      m_destroyFunc(std::move(other.m_destroyFunc)),
      m_queue(std::move(other.m_queue))
{
}

template <IsObject T>
ObjectPool<T>& ObjectPool<T>::operator=(ObjectPool&& other) noexcept
{
    if (this != &other) {
        Clear();
        m_createFunc = std::move(other.m_createFunc);
        m_destroyFunc = std::move(other.m_destroyFunc);
        m_queue = std::move(other.m_queue);
    }
    return *this;
}

template <IsObject T>
ObjectPool<T>::~ObjectPool()
{
    Clear();
}

template <IsObject T>
std::unique_ptr<T, std::function<void(T*)>> ObjectPool<T>::Acquire()
{
    T* obj = nullptr;
    if (!m_queue.try_dequeue(obj)) {
        obj = m_createFunc();
    }
    obj->Reset();
    return std::unique_ptr<T, std::function<void(T*)>>(obj, [this](T* p) { Destroy(p); });
}

template <IsObject T>
bool ObjectPool<T>::Preallocate(uint32_t count)
{
    try {
        for (uint32_t i = 0; i < count; ++i) {
            m_queue.enqueue(m_createFunc());
        }
        return true;
    } catch (...) {
        return false;
    }
}

template <IsObject T>
size_t ObjectPool<T>::Size() const
{
    return m_queue.size_approx();
}

template <IsObject T>
void ObjectPool<T>::Clear()
{
    T* obj = nullptr;
    while (m_queue.try_dequeue(obj)) {
        m_destroyFunc(obj);
    }
}

template <IsObject T>
void ObjectPool<T>::Destroy(T* obj)
{
    if (obj != nullptr) {
        try {
            m_queue.enqueue(obj);
        } catch (...) {
            m_destroyFunc(obj);
        }
    }
}

} // namespace galay::utils

#endif // GALAY_POOL_TCC