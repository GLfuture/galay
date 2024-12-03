#include "Time.h"

namespace galay::time
{

int64_t GetCurrentTime()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

std::string GetCurrentGMTTimeString()
{
    std::time_t now = std::time(nullptr);
    std::tm* gmt_time = std::gmtime(&now);
    if (gmt_time == nullptr) {
        return "";
    }
    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", gmt_time);
    return buffer;
}

}