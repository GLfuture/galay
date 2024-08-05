#include "waitgroup.h"
galay::common::WaitGroup::WaitGroup()
{
    this->m_coNum.store(0);
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
        this->m_awaiter = common::GroupAwaiter(false);
    }else{
        this->m_awaiter = common::GroupAwaiter(true);
    }
    return this->m_awaiter;
}


void 
galay::common::WaitGroup::Done()
{
    if(this->m_coNum.load() > 0) this->m_coNum.fetch_sub(1);
    if(this->m_coNum.load() == 0){
        this->m_awaiter.Resume();
    }
}
