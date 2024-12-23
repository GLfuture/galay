#ifndef GALAY_EXTERNAPI_TCC
#define GALAY_EXTERNAPI_TCC

#include "ExternApi.hpp"

namespace galay 
{

template <VecBase IOVecType>
inline IOVecHolder<IOVecType>::IOVecHolder(size_t size) 
{ 
    VecMalloc(&m_vec, size); 
}


template <VecBase IOVecType>
inline IOVecHolder<IOVecType>::IOVecHolder(std::string&& buffer) 
{
    if(!buffer.empty()) {
        Reset(std::move(buffer));
    }
}

template <VecBase IOVecType>
inline bool IOVecHolder<IOVecType>::Realloc(size_t size)
{
    return VecRealloc(&m_vec, size);
}

template <VecBase IOVecType>
inline void IOVecHolder<IOVecType>::ClearBuffer()
{
    if(m_vec.m_buffer == nullptr) return;
    memset(m_vec.m_buffer, 0, m_vec.m_size);
    m_vec.m_offset = 0;
    m_vec.m_length = 0;
}

template <VecBase IOVecType>
inline bool IOVecHolder<IOVecType>::Reset(const IOVecType& iov)
{
    if(iov.m_buffer == nullptr) return false;
    FreeMemory();
    m_vec.m_buffer = iov.m_buffer;
    m_vec.m_size = iov.m_size;
    m_vec.m_offset = iov.m_offset;
    m_vec.m_length = iov.m_length;
    return true;
}

template <VecBase IOVecType>
inline bool IOVecHolder<IOVecType>::Reset()
{
    return FreeMemory();
}

template <VecBase IOVecType>
inline bool IOVecHolder<IOVecType>::Reset(std::string&& str)
{
    FreeMemory();
    m_temp = std::forward<std::string>(str);
    m_vec.m_buffer = m_temp.data();
    m_vec.m_size = m_temp.length();
    return true;
}

template <VecBase IOVecType>
inline IOVecType* IOVecHolder<IOVecType>::operator->()
{
    return &m_vec;
}

template <VecBase IOVecType>
inline IOVecType* IOVecHolder<IOVecType>::operator &()
{
    return &m_vec;
}

template <VecBase IOVecType>
inline IOVecHolder<IOVecType>::~IOVecHolder()
{
    FreeMemory();    
}

template <VecBase IOVecType>
inline bool IOVecHolder<IOVecType>::VecMalloc(IOVecType* src, size_t size)
{
    if (src == nullptr || src->m_size != 0 || src->m_buffer != nullptr) {
        return false;
    }
    src->m_buffer = static_cast<char*>(calloc(size, sizeof(char)));
    if (src->m_buffer == nullptr) {
        return false;
    }
    src->m_size = size;
    return true;
}

template <VecBase IOVecType>
inline bool IOVecHolder<IOVecType>::VecRealloc(IOVecType* src, size_t size)
{
    if (src == nullptr || src->m_buffer == nullptr) {
        return false;
    }
    src->m_buffer = static_cast<char*>(realloc(src->m_buffer, size));
    if (src->m_buffer == nullptr) {
        return false;
    }
    src->m_size = size;
    return true;
}

template <VecBase IOVecType>
inline bool IOVecHolder<IOVecType>::VecFree(IOVecType* src)
{
    if(src == nullptr || src->m_size == 0 || src->m_buffer == nullptr) {
        return false;
    }
    free(src->m_buffer);
    src->m_size = 0;
    src->m_length = 0;
    src->m_buffer = nullptr;
    src->m_offset = 0;
    return true;
}


template <VecBase IOVecType>
inline bool IOVecHolder<IOVecType>::FreeMemory() 
{
    bool res = false;
    if(m_temp.empty()) {
        return VecFree(&m_vec);
    } else {
        if(m_temp.data() != m_vec.m_buffer) {
            res = VecFree(&m_vec);
        }
    }
    m_temp.clear();
    return true;
}

template<>
inline bool IOVecHolder<UdpIOVec>::Reset(const UdpIOVec& iov)
{
    if(iov.m_buffer == nullptr) return false;
    FreeMemory();
    m_vec.m_buffer = iov.m_buffer;
    m_vec.m_size = iov.m_size;
    m_vec.m_offset = iov.m_offset;
    m_vec.m_length = iov.m_length;
    m_vec.m_addr = iov.m_addr;
    return true;
}

template<LoadBalancerType Type>
inline void SchedulerHolder<Type>::Initialize(uint32_t scheduler_size, int timeout)
{
    if(scheduler_size == 0) scheduler_size = 1;
    std::vector<typename Type::value_type*> scheduler_ptrs;
    for (int i = 0; i < scheduler_size; ++i)
    {
        int try_count = MAX_START_SCHEDULERS_RETRY;
        m_schedulers.emplace_back(std::make_unique<typename Type::value_type>());
        scheduler_ptrs.emplace_back(m_schedulers[i].get());
        m_schedulers[i]->Loop(timeout);
        while(!m_schedulers[i]->IsRunning() && try_count-- >= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        if(try_count <= 0) {
            LogTrace("Start coroutine scheduler failed, index: {}", i);
            exit(-1);
        }
    }
    SchedulerLoadBalancer = std::make_unique<Type>(scheduler_ptrs);
}


template<LoadBalancerType Type>
inline void SchedulerHolder<Type>::Initialize(std::vector<std::unique_ptr<typename Type::value_type>>&& schedulers)
{
    m_schedulers = std::forward<decltype(schedulers)>(schedulers);
    std::vector<typename Type::value_type*> scheduler_ptrs;
    scheduler_ptrs.reserve(m_schedulers.size());
    for(auto& scheduler : m_schedulers) {
        scheduler_ptrs.push_back(scheduler.get());
    }
    SchedulerLoadBalancer = std::make_unique<Type>(scheduler_ptrs);
}

template<LoadBalancerType Type>
inline void SchedulerHolder<Type>::Destroy()
{
    for( auto& scheduler : m_schedulers) {
        scheduler->Stop();
    }
}

template<LoadBalancerType Type>
inline SchedulerHolder<Type>* SchedulerHolder<Type>::GetInstance()
{
    if(!Instance) {
        Instance = std::make_unique<SchedulerHolder>();
    }
    return Instance.get();
}

template<LoadBalancerType Type>
inline typename Type::value_type* SchedulerHolder<Type>::GetScheduler()
{
    return SchedulerLoadBalancer.get()->Select();
}

template<LoadBalancerType Type>
inline typename Type::value_type* SchedulerHolder<Type>::GetScheduler(uint32_t index)
{
    return m_schedulers[index].get();
}

}


#endif