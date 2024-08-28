#include "waitgroup.h"

namespace galay::coroutine
{

WaitGroup::WaitGroup()
{
    this->m_coNum.store(0);
}

void 
WaitGroup::Add(int num)
{
    this->m_coNum.fetch_add(num);
}

GroupAwaiter& 
WaitGroup::Wait()
{
    if(this->m_coNum.load() == 0) {
        this->m_awaiter = coroutine::GroupAwaiter(false);
    }else{
        this->m_awaiter = coroutine::GroupAwaiter(true);
    }
    return this->m_awaiter;
}


void 
WaitGroup::Done()
{
    if(this->m_coNum.load() > 0) this->m_coNum.fetch_sub(1);
    if(this->m_coNum.load() == 0){
        this->m_awaiter.Resume();
    }
}

}