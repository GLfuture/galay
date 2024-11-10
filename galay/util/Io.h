#ifndef __GALAY_IO_H__
#define __GALAY_IO_H__

#include "../common/Base.h"
#include <string>
#include <string.h>

namespace galay::io::file
{
    class SyncFileStream
    {
    public:
        static std::string ReadFile(const std::string &FileName , bool IsBinary = false);
        static void WriteFile(const std::string &FileName, const std::string &Content , bool IsBinary = false);
    };

    class ZeroCopyFile
    {
    public:
    #if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) 
        static std::string ReadFile(const std::string &FileName);
        static void WriteFile(const std::string &FileName, const std::string &Content,bool IsBinary = false);
    #endif
    };
}

#endif