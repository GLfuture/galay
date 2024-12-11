#ifndef __GALAY_OBJECT_POOL_HPP__
#define __GALAY_OBJECT_POOL_HPP__

#include <concurrentqueue/moodycamel/concurrentqueue.h>
#include <cinttypes>
#include <concepts>

namespace galay::utils
{

template <typename T>
concept Object = requires(T t)
{
    { t.Reset() };
};

//not thread safe
template<Object T>
class ObjectPoolMutiThread
{
public:
    ObjectPoolMutiThread(uint32_t capacity)
        : m_capacity(capacity)
    {
        for(uint32_t i = 0; i < capacity; ++i) {
            m_queue.enqueue(new T());
        }
    }

    T* GetObjector()
    {
        if(T* objector = nullptr; m_queue.try_dequeue(objector)){
            return objector;
        }
        return new T();
    }

    // to keep capacity fixed
    void ReturnObjector(T* objector)
    {
        objector->Reset();
        if(m_queue.size_approx() >= m_capacity.load()){
            m_queue.enqueue(objector);
        }else{
            delete objector;
        }
    }

    ~ObjectPoolMutiThread()
    {
        T* objector = nullptr;
        while(m_queue.try_dequeue(objector)){
            delete objector;
        }
    }

private:
    std::atomic_uint32_t m_capacity;
    moodycamel::ConcurrentQueue<T*> m_queue;
};

}

#endif