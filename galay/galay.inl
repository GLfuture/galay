#ifndef GALAY_INL
#define GALAY_INL

#include "galay.hpp"


namespace galay
{

inline GalayEnvConf::ptr GalayEnvConf::Create()
{
    return std::make_shared<GalayEnvConf>();
}

inline GalayEnv::GalayEnv(GalayEnvConf conf)
{
    InitializeGalayEnv(conf.m_coroutineSchedulerConf, conf.m_eventSchedulerConf, conf.m_sessionSchedulerConf);
}

inline GalayEnv::~GalayEnv()
{
    DestroyGalayEnv();
}

}

#endif