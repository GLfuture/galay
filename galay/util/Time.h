#ifndef __GALAY_TIME_H__
#define __GALAY_TIME_H__

#include <chrono>
#include <string>

namespace galay::time
{
    
extern int64_t GetCurrentTime(); 
extern std::string GetCurrentGMTTimeString();

}

#endif