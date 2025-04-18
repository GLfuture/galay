#include "Log.h"
#include <spdlog/async.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

namespace galay
{

Logger::uptr Logger::CreateStdoutLoggerST(const std::string &name)
{
    auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_st>();
    auto spd_logger = std::make_shared<spdlog::logger>(name, sink);
    return std::make_unique<Logger>(spd_logger);
}

Logger::uptr Logger::CreateStdoutLoggerMT(const std::string &name)
{
    auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto spd_logger = std::make_shared<spdlog::logger>(name, sink);
    return std::make_unique<Logger>(spd_logger);
}

Logger::uptr Logger::CreateDailyFileLoggerST(const std::string &name, const std::string &file_path, int hour, int minute)
{
    auto sink = std::make_shared<spdlog::sinks::daily_file_sink_st>(
        file_path, hour, minute, false, 0);
    auto spd_logger = std::make_shared<spdlog::logger>(name, sink);
    return std::make_unique<Logger>(spd_logger);
}

Logger::uptr Logger::CreateDailyFileLoggerMT(const std::string &name, const std::string &file_path, int hour, int minute)
{
    auto sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
        file_path, hour, minute, false, 0);
    auto spd_logger = std::make_shared<spdlog::logger>(name, sink);
    return std::make_unique<Logger>(spd_logger);
}

Logger::uptr Logger::CreateRotingFileLoggerST(const std::string &name, const std::string &file_path, size_t max_size, size_t max_files)
{
    auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_st>(
        file_path, max_size, max_files, false);
    auto spd_logger = std::make_shared<spdlog::logger>(name, sink);
    return std::make_unique<Logger>(spd_logger);
}

Logger::uptr Logger::CreateRotingFileLoggerMT(const std::string &name, const std::string &file_path, size_t max_size, size_t max_files)
{
    auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        file_path, max_size, max_files, false);
    auto spd_logger = std::make_shared<spdlog::logger>(name, sink);
    return std::make_unique<Logger>(spd_logger);
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

Logger::~Logger()
{
}

}

namespace galay::details
{
std::unique_ptr<InternelLogger> InternelLogger::m_instance = nullptr;

InternelLogger::InternelLogger() {
    m_thread_pool = std::make_shared<spdlog::details::thread_pool>( DEFAULT_LOG_QUEUE_SIZE, DEFAULT_LOG_THREADS);
    auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(DEFAULT_LOG_FILE_PATH, DEFAULT_MAX_LOG_FILE_SIZE, DEFAULT_MAX_LOG_FILES);
    auto logger = std::make_shared<spdlog::async_logger>("galay", sink, m_thread_pool, spdlog::async_overflow_policy::overrun_oldest);
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%f][%^%L%$][%t][%25!s:%4!#][%20!!] %v");
    logger->set_level(spdlog::level::debug);
    logger->flush_on(spdlog::level::debug);
    m_logger = std::make_unique<Logger>(logger); 
}

InternelLogger *InternelLogger::GetInstance()
{
    if(m_instance == nullptr) {
        m_instance = std::make_unique<InternelLogger>();
    }
    return m_instance.get();
}

void InternelLogger::SetLogger(Logger::uptr logger)
{
    m_logger = std::move(logger);
}

Logger* InternelLogger::GetLogger()
{
    return m_logger.get();
}


void InternelLogger::Shutdown() {
    if (m_instance) {
        m_instance->m_logger->SpdLogger()->flush();
        m_instance->m_logger.reset();
        // 再释放线程池（此时线程池引用计数可能已为 0）
        m_instance->m_thread_pool.reset();
        m_instance.reset(); 
    }
}

InternelLogger::~InternelLogger()
{
}
}
