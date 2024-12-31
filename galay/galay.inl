#ifndef GALAY_INL
#define GALAY_INL

#include "galay.hpp"


namespace galay
{

inline GalayEnv::GalayEnv(std::pair<uint32_t, int> coroutineConf, std::pair<uint32_t, int> eventConf, std::pair<uint32_t, int> timerConf)
{
    InitializeGalayEnv(coroutineConf, eventConf, timerConf);
}

inline GalayEnv::~GalayEnv()
{
    DestroyGalayEnv();
}

}

#endif