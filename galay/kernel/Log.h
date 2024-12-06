#ifndef GALAY_LOG_H
#define GALAY_LOG_H

#ifdef ENABLE_DISPLAY_GALAY_LOG
    #define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#endif

#include <memory>
#include <spdlog/spdlog.h>

namespace galay::details {

#define DEFAULT_LOG_QUEUE_SIZE      8192
#define DEFAULT_LOG_THREADS         2

#define DEFAULT_LOG_FILE_PATH       "galay.log"
#define DEFAULT_MAX_LOG_FILE_SIZE   (10 * 1024 * 1024)
#define DEFAULT_MAX_LOG_FILES       3



class Logger {
public: 
    Logger();
    static Logger* GetInstance();
    std::shared_ptr<spdlog::logger>& GetLogger();
private:
    static std::unique_ptr<Logger> m_instance;
    std::shared_ptr<spdlog::logger> m_logger;
};

}

#define LogTrace(...)       SPDLOG_LOGGER_TRACE(galay::details::Logger::GetInstance()->GetLogger(), __VA_ARGS__)
#define LogDebug(...)       SPDLOG_LOGGER_DEBUG(galay::details::Logger::GetInstance()->GetLogger(), __VA_ARGS__)
#define LogInfo(...)        SPDLOG_LOGGER_INFO(galay::details::Logger::GetInstance()->GetLogger(), __VA_ARGS__)
#define LogWarn(...)        SPDLOG_LOGGER_WARN(galay::details::Logger::GetInstance()->GetLogger(), __VA_ARGS__)
#define LogError(...)       SPDLOG_LOGGER_ERROR(galay::details::Logger::GetInstance()->GetLogger(), __VA_ARGS__)
#define LogCritical(...)    SPDLOG_LOGGER_CRITICAL(galay::details::Logger::GetInstance()->GetLogger(), __VA_ARGS__)

#endif