#ifndef GALAY_FILE_H
#define GALAY_FILE_H

#include <string>

namespace galay
{
    namespace Helper
    {
        class FileOP
        {
        public:
            static std::string ReadFile(const std::string &FileName);
            static void WriteFile(const std::string &FileName, const std::string &Content);
        };

    }

} // namespace galay

#endif