#include "reflection.h"
#include <iostream>

galay::common::GY_FactoryManager::uptr galay::common::GY_FactoryManager::m_FactoryManager;

void 
galay::common::GY_FactoryManager::AddReleaseFunc(std::function<void()> func)
{
    if(!m_FactoryManager){
        m_FactoryManager = std::make_unique<GY_FactoryManager>();
    }
    m_FactoryManager->m_ReleaseFunc.push_back(func);
}

galay::common::GY_FactoryManager::~GY_FactoryManager()
{
    if(!m_ReleaseFunc.empty()){
        for(auto& v: m_ReleaseFunc){
            v();
        }
        m_ReleaseFunc.clear();
    }
}

