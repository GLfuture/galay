#include "waitgroup.h"
galay::common::WaitGroup::WaitGroup(std::weak_ptr<common::GY_NetCoroutinePool> coPool)
{
    this->m_coNum.store(0);
    this->m_coPool = coPool;
}

void 
galay::common::WaitGroup::Add(int num)
{
    this->m_coNum.fetch_add(num);
}

galay::common::GroupAwaiter& 
galay::common::WaitGroup::Wait()
{
    if(this->m_coNum.load() == 0) {
        this->m_awaiter = common::GroupAwaiter(m_coPool,false);
    }else{
        this->m_awaiter = common::GroupAwaiter(m_coPool,true);
    }
    return this->m_awaiter;
}


void 
galay::common::WaitGroup::Done()
{
    this->m_coNum.fetch_sub(1);
    if(this->m_coNum.load() == 0){
        this->m_awaiter.Resume();
    }
}
