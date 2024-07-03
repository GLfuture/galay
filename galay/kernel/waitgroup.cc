#include "waitgroup.h"
galay::kernel::WaitGroup::WaitGroup(std::weak_ptr<common::GY_NetCoroutinePool> coPool)
{
    this->m_coNum.store(0);
    this->m_coPool = coPool;
}

void 
galay::kernel::WaitGroup::Add(int num)
{
    this->m_coNum.fetch_add(num);
}

galay::kernel::GroupAwaiter& 
galay::kernel::WaitGroup::Wait()
{
    if(this->m_coNum.load() == 0) {
        this->m_awaiter = GroupAwaiter(m_coPool,false);
    }else{
        this->m_awaiter = GroupAwaiter(m_coPool,true);
    }
    return this->m_awaiter;
}


void 
galay::kernel::WaitGroup::Done()
{
    this->m_coNum.fetch_sub(1);
    if(this->m_coNum.load() == 0){
        this->m_awaiter.Resume();
    }
}
