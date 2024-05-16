#include "countsemaphore.h"
#include <assert.h>



galay::util::CountSemaphore::CountSemaphore(uint64_t initcount,uint64_t capacity)
{
    assert(initcount <= capacity);
    m_capacity = capacity;
    m_nowcount = initcount;
}

bool 
galay::util::CountSemaphore::Get(uint64_t count)
{
    std::unique_lock<std::mutex> lock(m_mtx);
    if(count > m_capacity) return false;
    m_cond.wait(lock,[this,count](){
        return m_nowcount >= count;
    });
    m_nowcount -= count;
    return true;
}


void 
galay::util::CountSemaphore::Put(uint64_t count)
{
    std::unique_lock<std::mutex> lock(m_mtx);
    //可以m_nowcount += count，因为Get和Put互斥，+=时Get无法进入，不存在m_nowcount > m_capacity时大流量包的误判
    m_nowcount += count;
    if(m_nowcount > m_capacity) m_nowcount = m_capacity;
    lock.unlock();
    m_cond.notify_all();
}