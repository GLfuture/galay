#ifndef GALAY_SYSTEM_H
#define GALAY_SYSTEM_H

#include "galay/common/Base.h"
#include <string>
#include <string.h>

namespace galay::utils
{

extern int64_t GetCurrentTimeMs(); 
extern std::string GetCurrentGMTTimeString();

extern std::string ReadFile(const std::string &FileName , bool IsBinary = false);
extern void WriteFile(const std::string &FileName, const std::string &Content , bool IsBinary = false);

#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) 
extern std::string ZeroReadFile(const std::string &FileName);
extern void ZeroWriteFile(const std::string &FileName, const std::string &Content,bool IsBinary = false);
#endif

extern std::string GetEnvValue(const std::string &name);

}

#endif