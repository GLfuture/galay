#ifndef __GALAY_OBJECT_POOL_HPP__
#define __GALAY_OBJECT_POOL_HPP__

#include <queue>
#include <cinttypes>

namespace galay::util
{

template <typename T>
concept Object = requires(T t)
{
    { t.Reset() };
};

//not thread safe
template<Object T>
class ObjectPool
{
public:
    ObjectPool(uint32_t capacity)
        : m_capacity(capacity)
    {
        for(int i = 0; i < capacity; i++)
        {
            m_pool.push_back(new T());
        }
    }

    T* Get()
    {
        if(m_pool.empty())
        {
            return new T();
        }
        else
        {
            T* ptr = m_pool.front();
            m_pool.pop();
            return ptr;
        }
    }

    // to keep capacity fixed
    void FixedPut(T* ptr)
    {
        if(m_pool.size() < m_capacity)
        {
            m_pool.push(ptr);
        }
        else
        {
            delete ptr;
        }
    }

    //if queue's size >= capacity, capacity will increase
    void ActivePut(T* ptr)
    {
        if(m_pool.size() < m_capacity)
        {
            m_pool.push(ptr);
        }
        else
        {
            m_pool.push(ptr);
            ++ m_capacity;
        }
    }

    void DynamicAdjust(uint32_t capacity)
    {
        m_capacity = capacity;
        while (m_capacity < m_pool.size())
        {
            delete m_pool.front();
            m_pool.pop();
        }
    }

    ~ObjectPool()
    {
        while(!m_pool.empty())
        {
            delete m_pool.front();
            m_pool.pop();
        }
    }

private:
    uint32_t m_capacity;
    std::queue<T*> m_pool;
};

}

#endif