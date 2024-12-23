#ifndef GALAY_EXTERNAPI_TCC
#define GALAY_EXTERNAPI_TCC

#include "ExternApi.hpp"

namespace galay 
{

template <VecBase IOVecType>
IOVecHolder<IOVecType>::IOVecHolder(size_t size) 
{ 
    VecMalloc(&m_vec, size); 
}


template <VecBase IOVecType>
IOVecHolder<IOVecType>::IOVecHolder(std::string&& buffer) 
{
    if(!buffer.empty()) {
        Reset(std::move(buffer));
    }
}

template <VecBase IOVecType>
bool IOVecHolder<IOVecType>::Realloc(size_t size)
{
    return VecRealloc(&m_vec, size);
}



}


#endif