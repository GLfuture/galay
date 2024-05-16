#ifndef GALAY_FILE_H
#define GALAY_FILE_H

#include <string>

namespace galay
{
    namespace IOFunction
    {
        namespace FileIOFunction{
            class SyncFileStream
            {
            public:
                static ::std::string ReadFile(const ::std::string &FileName , bool IsBinary = false);
                static void WriteFile(const ::std::string &FileName, const ::std::string &Content , bool IsBinary = false);
            };

            class ZeroCopyFile
            {
            public:
            #ifdef __linux__
                static ::std::string ReadFile(const ::std::string &FileName);
                static void WriteFile(const ::std::string &FileName, const ::std::string &Content,bool IsBinary = false);
            #endif
            };
        }

    }

} // namespace galay

#endif