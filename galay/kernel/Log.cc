#include "Log.h"
#include <spdlog/async.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

namespace galay
{

Logger::ptr Logger::CreateStdoutLoggerST(const std::string &name)
{
    return std::make_shared<Logger>(spdlog::stdout_color_st(name));
}

Logger::ptr Logger::CreateStdoutLoggerMT(const std::string &name)
{
    return std::make_shared<Logger>(spdlog::stdout_color_mt(name));
}

Logger::ptr Logger::CreateDailyFileLoggerST(const std::string &name, const std::string &file_path, int hour, int minute)
{
    return std::make_shared<Logger>(spdlog::daily_logger_st(name, file_path, hour, minute));
}

Logger::ptr Logger::CreateDailyFileLoggerMT(const std::string &name, const std::string &file_path, int hour, int minute)
{
    return std::make_shared<Logger>(spdlog::daily_logger_mt(name, file_path, hour, minute));
}

Logger::ptr Logger::CreateRotingFileLoggerST(const std::string &name, const std::string &file_path, size_t max_size, size_t max_files)
{
    return std::make_shared<Logger>(spdlog::rotating_logger_st(name, file_path, max_size, max_files));
}

Logger::ptr Logger::CreateRotingFileLoggerMT(const std::string &name, const std::string &file_path, size_t max_size, size_t max_files)
{
    return std::make_shared<Logger>(spdlog::rotating_logger_mt(name, file_path, max_size, max_files));  
}

Logger::Logger(std::shared_ptr<spdlog::logger> logger)
    : m_logger(logger)
{
}


std::shared_ptr<spdlog::logger> Logger::SpdLogger()
{
    return m_logger;
}

Logger &Logger::Pattern(const std::string &pattern)
{
    m_logger->set_pattern(pattern);
    return *this;
}

Logger &Logger::Level(spdlog::level::level_enum level)
{
    m_logger->set_level(level);
    return *this;
}
}

namespace galay::details
{
std::unique_ptr<InternelLogger> InternelLogger::m_instance = nullptr;

InternelLogger::InternelLogger() {
    spdlog::init_thread_pool(DEFAULT_LOG_QUEUE_SIZE, DEFAULT_LOG_THREADS);
    auto logger = std::make_shared<spdlog::logger>("galay", std::make_shared<spdlog::sinks::rotating_file_sink_mt>(DEFAULT_LOG_FILE_PATH, DEFAULT_MAX_LOG_FILE_SIZE, DEFAULT_MAX_LOG_FILES));
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%f][%L][%t][%25!s:%4!#][%20!!] %v");
    logger->set_level(spdlog::level::debug);
    logger->flush_on(spdlog::level::debug);
}

InternelLogger *InternelLogger::GetInstance()
{
    if(m_instance == nullptr) {
        m_instance = std::make_unique<InternelLogger>();
    }
    return m_instance.get();
}

void InternelLogger::SetLogger(Logger::ptr logger)
{
    m_logger = logger;
}

Logger::ptr InternelLogger::GetLogger()
{
    return m_logger;
}
}

