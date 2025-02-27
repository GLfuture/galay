#ifndef GALAY_POOL_TCC
#define GALAY_POOL_TCC

#include "Pool.hpp"

namespace galay::pool
{


template<Object T>
inline ProtocolPool<T>::ProtocolPool(uint32_t capacity)
    : m_capacity(capacity)
{
    for(uint32_t i = 0; i < capacity; ++i) {
        m_queue.enqueue(std::make_unique<T>());
    }
}

template<Object T>
inline std::unique_ptr<T> ProtocolPool<T>::GetProtocol()
{
    if(std::unique_ptr<T> objector = nullptr; m_queue.try_dequeue(objector)){
        return objector;
    }
    return std::make_unique<T>();
}

template<Object T>
inline void ProtocolPool<T>::PutProtocol(std::unique_ptr<T> objector)
{
    objector->Reset();
    if(m_queue.size_approx() >= m_capacity.load()){
        m_queue.enqueue(std::move(objector));
    }
}

template<Object T>
inline ProtocolPool<T>::~ProtocolPool()
{
    std::unique_ptr<T> objector = nullptr;
    while(m_queue.try_dequeue(objector)){
    }
}


template <typename T>
inline SessionPool<T>::SessionPool(uint32_t capacity)
{
}

template <typename T>
inline SessionPool<T>::SessionPool(std::vector<T *> sessions)
{
}

template <typename T>
inline T *SessionPool<T>::GetSession()
{
    return nullptr;
}


template <typename T>
inline void SessionPool<T>::PutSession(T *session)
{
}

template <typename T>
inline SessionPool<T>::~SessionPool()
{
}
}

#endif