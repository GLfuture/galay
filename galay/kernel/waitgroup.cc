#include "waitgroup.h"
galay::WaitGroup::WaitGroup()
{
    this->m_coNum.store(0);
}

void 
galay::WaitGroup::Add(int num)
{
    this->m_coNum.fetch_add(num);
}

galay::GroupAwaiter& 
galay::WaitGroup::Wait()
{
    if(this->m_coNum.load() == 0) {
        this->m_awaiter = GroupAwaiter(false);
    }else{
        this->m_awaiter = GroupAwaiter(true);
    }
    return this->m_awaiter;
}


void 
galay::WaitGroup::Done()
{
    this->m_coNum.fetch_sub(1);
    if(this->m_coNum.load() == 0){
        this->m_awaiter.Resume();
    }
}
