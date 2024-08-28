#include "reflection.h"

galay::common::FactoryManager::uptr galay::common::FactoryManager::m_FactoryManager;

void 
galay::common::FactoryManager::AddReleaseFunc(std::function<void()> func)
{
    if(!m_FactoryManager){
        m_FactoryManager = std::make_unique<FactoryManager>();
    }
    m_FactoryManager->m_ReleaseFunc.push_back(func);
}

galay::common::FactoryManager::~FactoryManager()
{
    if(!m_ReleaseFunc.empty()){
        for(auto& v: m_ReleaseFunc){
            v();
        }
        m_ReleaseFunc.clear();
    }
}

