#include "Log.h"
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>

namespace galay::details
{
std::unique_ptr<Logger> Logger::m_instance = nullptr;

Logger::Logger() {
    spdlog::init_thread_pool(DEFAULT_LOG_QUEUE_SIZE, DEFAULT_LOG_THREADS);
    m_logger = std::make_shared<spdlog::logger>("galay", std::make_shared<spdlog::sinks::rotating_file_sink_mt>(DEFAULT_LOG_FILE_PATH, DEFAULT_MAX_LOG_FILE_SIZE, DEFAULT_MAX_LOG_FILES));
    m_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%f][%L][%t][%25!s:%4!#][%20!!] %v");
    m_logger->set_level(spdlog::level::debug);
    m_logger->flush_on(spdlog::level::debug);
}

Logger *Logger::GetInstance()
{
    if(m_instance == nullptr) {
        m_instance = std::make_unique<Logger>();
    }
    return m_instance.get();
}

std::shared_ptr<spdlog::logger>& Logger::GetLogger()
{
    return m_logger;
}


}