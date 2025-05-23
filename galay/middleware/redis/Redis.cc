#include "Redis.hpp"
#include "galay/kernel/ExternApi.hpp"
#include "galay/kernel/Async.hpp"
#include "galay/utils/System.h"
#include <regex>

namespace galay::details
{
RedisEvent::RedisEvent(redis::RedisAsyncSession* redis)
    : m_redis(redis), m_ctx(nullptr)
{
}

bool RedisEvent::SetEventEngine(EventEngine *engine)
{
    m_engine = engine;
    return true;
}

EventType RedisEvent::GetEventType()
{
    switch (m_redisWaitType)
    {
    case RedisWaitEventType_None:
        return kEventTypeNone;
    case RedisWaitEventType_Write:
        return kEventTypeWrite;
    case RedisWaitEventType_Read:
        return kEventTypeRead;
    default:
        break;
    }
    return kEventTypeNone;
}

GHandle RedisEvent::GetHandle()
{
    return {m_redis->m_redis->c.fd};
}

void RedisEvent::HandleEvent(EventEngine *engine)
{
    switch (m_redisWaitType)
    {
    case RedisWaitEventType_None:
        break;
    case RedisWaitEventType_Write:
        HandleRedisWrite(engine);
        break;
    case RedisWaitEventType_Read:
        HandleRedisRead(engine);
        break;
    default:
        break;
    }
}

std::string RedisEvent::Name()
{
    switch (m_redisWaitType)
    {
    case RedisWaitEventType_None:
        return "RedisNoneEvent";
    case RedisWaitEventType_Write:
        return "RedisWriteEvent";
    case RedisWaitEventType_Read:
        return "RedisReadEvent";
    default:
        break;
    }
    return "";
}

bool RedisEvent::OnWaitPrepare(CoroutineBase::wptr co, void *ctx)
{
    this->m_waitco = co;
    this->m_ctx = ctx;
    switch (m_redisWaitType)
    {
    case RedisWaitEventType_None:
        break;
    case RedisWaitEventType_Read:
        return OnWaitReadPrepare(co, ctx);
    case RedisWaitEventType_Write:
        return OnWaitWritePrepare(co, ctx);
    default:
        break;
    }
    return false;
}

AwaiterBase *RedisEvent::GetAwaiterBase()
{
    return m_waitco.lock()->GetAwaiter();
}

void RedisEvent::ToResume()
{
    m_waitco.lock()->GetCoScheduler()->ToResumeCoroutine(m_waitco);
}

bool RedisEvent::OnWaitWritePrepare(CoroutineBase::wptr co, void *ctx)
{
    return true;
}

bool RedisEvent::OnWaitReadPrepare(CoroutineBase::wptr co, void *ctx)
{
    return true;
}

bool RedisEvent::HandleRedisWrite(EventEngine *engine)
{
    m_redisWaitType = RedisWaitEventType_Read;
    engine->ModEvent(this, nullptr);
    redisAsyncHandleWrite(m_redis->m_redis);
    return true;
}

bool RedisEvent::HandleRedisRead(EventEngine *engine)
{
#ifdef USE_EPOLL
    m_redisWaitType = RedisWaitEventType_Read;
    engine->ModEvent(this, nullptr);
#elif defined(USE_KQUEUE)
#endif
    redisAsyncHandleRead(m_redis->m_redis);
    return true;
}

RedisEvent::~RedisEvent()
{
    if(m_engine) m_engine.load()->DelEvent(this, nullptr);
}


}

namespace galay::redis 
{

RedisConfig::ptr RedisConfig::CreateConfig()
{
    return std::make_shared<RedisConfig>();
}


void RedisConfig::ConnectWithTimeout(uint64_t timeout)
{
    m_params = timeout;
    m_connection_option = RedisConnectionOption::kRedisConnectionWithTimeout;
}

void RedisConfig::ConnectWithBind(const std::string &addr)
{
    m_params = addr;
    m_connection_option = RedisConnectionOption::kRedisConnectionWithBind;
}

void RedisConfig::ConnectWithBindAndReuse(const std::string &addr)
{
    m_params = addr;
    m_connection_option = RedisConnectionOption::kRedisConnectionWithBindAndReuse;
}

void RedisConfig::ConnectWithUnix(const std::string &path)
{
    m_params = path;
    m_connection_option = RedisConnectionOption::kRedisConnectionWithUnix;
}

void RedisConfig::ConnectWithUnixAndTimeout(const std::string &path, uint64_t timeout)
{
    m_params = std::make_pair(path, timeout);
    m_connection_option = RedisConnectionOption::kRedisConnectionWithUnixAndTimeout;
}

void RedisConfig::ConnectWithFd(int fd)
{
    m_params = fd;
    m_connection_option = RedisConnectionOption::kRedisConnectionWithFd;
}

RedisConnectionOption &RedisConfig::GetConnectOption()
{
    return m_connection_option;
}

std::any &RedisConfig::GetParams()
{
    return m_params;
}

RedisValue::RedisValue(RedisValue &&other)
{
    m_reply = std::move(other.m_reply);
    other.m_reply = nullptr;
    m_auto_free = other.m_auto_free;
    other.m_auto_free = false;
}

RedisValue &RedisValue::operator=(RedisValue &&other)
{
    m_reply = std::move(other.m_reply);
    other.m_reply = nullptr;
    m_auto_free = other.m_auto_free;
    other.m_auto_free = false;
    return *this;
}

bool RedisValue::IsNull()
{
    return m_reply->type == REDIS_REPLY_NIL || m_reply == nullptr;
}

bool RedisValue::IsStatus()
{
    return m_reply->type == REDIS_REPLY_STATUS;
}

std::string RedisValue::ToStatus()
{
    return std::string(m_reply->str, m_reply->len);
}

bool RedisValue::IsError()
{
    return m_reply->type == REDIS_REPLY_ERROR;
}

std::string RedisValue::ToError()
{
    return std::string(m_reply->str, m_reply->len);
}

bool RedisValue::IsInteger()
{
    return m_reply->type == REDIS_REPLY_INTEGER;
}

int64_t RedisValue::ToInteger()
{
    return m_reply->integer;
}

bool RedisValue::IsString()
{
    return m_reply->type == REDIS_REPLY_STRING ;
}

std::string RedisValue::ToString()
{
    std::string result(m_reply->str, m_reply->len);
    return result;
}

bool RedisValue::IsArray()
{
    return m_reply->type == REDIS_REPLY_ARRAY;
}

std::vector<RedisValue> RedisValue::ToArray()
{
    std::vector<RedisValue> result;
    result.reserve(m_reply->elements);
    for(size_t i = 0; i < m_reply->elements; ++i) {
        result.push_back(RedisValue(m_reply->element[i], false));
    }
    return std::move(result);
}

bool RedisValue::IsDouble()
{
    return m_reply->type == REDIS_REPLY_DOUBLE;
}

double RedisValue::ToDouble()
{
    return strtod(m_reply->str, nullptr);
}

bool RedisValue::IsBool()
{
    return m_reply->type == REDIS_REPLY_BOOL;
}

bool RedisValue::ToBool()
{
    return m_reply->integer == 1;
}

bool RedisValue::IsMap()
{
    return m_reply->type == REDIS_REPLY_MAP;
}

std::map<std::string, RedisValue> RedisValue::ToMap()
{
    std::map<std::string, RedisValue> result;
    for(size_t i = 0; i < m_reply->elements; i += 2) {
        result.emplace(std::string(m_reply->element[i]->str, m_reply->element[i]->len), RedisValue(m_reply->element[i + 1], false));
    }
    return std::move(result);
}

bool RedisValue::IsSet()
{
    return m_reply->type == REDIS_REPLY_SET;
}

std::vector<RedisValue> RedisValue::ToSet()
{
    std::vector<RedisValue> result;
    result.reserve(m_reply->elements);
    for(size_t i = 0; i < m_reply->elements; ++i) {
        result.push_back(RedisValue(m_reply->element[i], false));
    }
    return std::vector<RedisValue>();
}

bool RedisValue::IsAttr()
{
    return m_reply->type == REDIS_REPLY_ATTR;
}

bool RedisValue::IsPush()
{
    return m_reply->type == REDIS_REPLY_PUSH;
}

std::vector<RedisValue> RedisValue::ToPush()
{
    std::vector<RedisValue> result;
    result.reserve(m_reply->elements);
    for(size_t i = 0; i < m_reply->elements; ++i) {
        result.push_back(RedisValue(m_reply->element[i], false));
    }
    return std::move(result);
}

bool RedisValue::IsBigNumber()
{
    return m_reply->type == REDIS_REPLY_BIGNUM;
}

std::string RedisValue::ToBigNumber()
{
    std::string result(m_reply->str, m_reply->len);
    return result;
}

bool RedisValue::IsVerb()
{
    return m_reply->type == REDIS_REPLY_VERB;
}

std::string RedisValue::ToVerb()
{
    std::string result(m_reply->str, m_reply->len);
    return result;
}

RedisValue::~RedisValue()
{
    if(m_auto_free && m_reply) {
        freeReplyObject(m_reply);
        m_reply = nullptr;
    }
}

RedisAsyncValue::RedisAsyncValue(RedisAsyncValue &&other)
{
    m_reply = other.m_reply;
    other.m_reply = nullptr;
}

RedisAsyncValue &RedisAsyncValue::operator=(RedisAsyncValue &&other)
{
    m_reply = other.m_reply;
    other.m_reply = nullptr;
    return *this;
}

RedisSession::RedisSession(RedisConfig::ptr config)
    : m_config(config), m_logger(Logger::CreateStdoutLoggerMT("RedisLogger"))
{
    m_logger->Pattern("[%Y-%m-%d %H:%M:%S.%f][%L][%t][%25!s:%4!#][%20!!] %v").Level(spdlog::level::info);
}

RedisSession::RedisSession(RedisConfig::ptr config, Logger::uptr logger)
    : m_config(config), m_logger(std::move(logger))
{
    m_logger->Level(spdlog::level::info);
}

// redis://[password@]host[:port][/db_index]
bool RedisSession::Connect(const std::string &url)
{
    std::regex pattern(R"(^redis://(?:([^:@]*)(?::([^@]*))?@)?([a-zA-Z0-9\-\.]+)(?::(\d+))?(?:/(\d+))?$)");
    std::smatch matches;
    std::string username, password, host;
    int32_t port = 6379, db_index = 0;
    if (std::regex_match(url, matches, pattern)) {
        if (matches.size() > 1 && !matches[1].str().empty()) {
            username = matches[1];
        }
        if (matches.size() > 2 && !matches[2].str().empty()) {
            password = matches[2];
        }
        if (matches.size() > 3 && !matches[3].str().empty()) {
            host = matches[3];
        } else {
            RedisLogError(m_logger->SpdLogger(), "[Redis host is invalid]");
            return false;
        }
        if (matches.size() > 4 && !matches[4].str().empty()) {
            try
            {
                port = std::stoi(matches[4]);
            }
            catch(const std::exception& e)
            {
                RedisLogError(m_logger->SpdLogger(), "[Redis url is invalid]");
                return false;
            }
        }
        if (matches.size() > 5 && !matches[5].str().empty()) {
            try
            {
                db_index = std::stoi(matches[5]);
            }
            catch(const std::exception& e)
            {
                RedisLogError(m_logger->SpdLogger(), "[Redis url is invalid]");
                return false;
            }
        }
    
    } else {
        RedisLogError(m_logger->SpdLogger(), "[Redis url is invalid]");
        return false;
    }
    using namespace utils;
    std::string ip;
    switch (utils::CheckAddressType(host))
    {
    case AddressType::IPv4 :
        ip = host;
        break;
    case AddressType::IPv6 :
        throw std::runtime_error("unsupported address type: IPV6");
    case AddressType::Domain:
        ip = utils::GetHostIPV4(host);
        if (ip.empty()) {
            throw std::runtime_error("DNS resolution failed");
        }
        break;
    default:
        throw std::runtime_error("unsupported address type: INVALID");
    }
    return Connect(ip, port, username, password, db_index);
}

bool RedisSession::Connect(const std::string &ip, int32_t port, const std::string& username, const std::string &password)
{
    return Connect(ip, port, username, password, 0);
}

bool RedisSession::Connect(const std::string &ip, int32_t port, const std::string& username, const std::string &password, int32_t db_index)
{
    return Connect(ip, port, username, password, db_index, 2);
}

bool RedisSession::Connect(const std::string &ip, int32_t port, const std::string& username, const std::string &password, int32_t db_index, int version)
{
    std::string host = ip;
    if( host == "localhost") {
        host = "127.0.0.1";
    }
    switch (m_config->GetConnectOption())
    {
    case RedisConnectionOption::kRedisConnectionWithNull:
        m_redis = redisConnect(host.c_str(), port);
        break;
    case RedisConnectionOption::kRedisConnectionWithTimeout:
    {
        timeval timeout;
        uint64_t ms = std::any_cast<uint64_t>(m_config->GetParams());
        timeout.tv_sec = ms / 1000;
        timeout.tv_usec = (ms % 1000) * 1000;
        m_redis = redisConnectWithTimeout(host.c_str(), port, timeout);
    }
        break;
    case RedisConnectionOption::kRedisConnectionWithBind:
    {
        std::string addr = std::any_cast<std::string>(m_config->GetParams());
        redisOptions options;
        REDIS_OPTIONS_SET_TCP(&options, host.c_str(), port);
        options.endpoint.tcp.source_addr = addr.c_str();
        m_redis = redisConnectWithOptions(&options);
    }
        break;
    case RedisConnectionOption::kRedisConnectionWithBindAndReuse:
    {
        std::string addr = std::any_cast<std::string>(m_config->GetParams());
        redisOptions options;
        REDIS_OPTIONS_SET_TCP(&options, host.c_str(), port);
        options.endpoint.tcp.source_addr = addr.c_str();
        options.options |= REDIS_OPT_REUSEADDR;
        m_redis = redisConnectWithOptions(&options);
    }
        break;
    case RedisConnectionOption::kRedisConnectionWithUnix:
    {
        std::string path = std::any_cast<std::string>(m_config->GetParams());
        m_redis = redisConnectUnix(path.c_str());
    }
        break;
    case RedisConnectionOption::kRedisConnectionWithUnixAndTimeout:
    {
        auto params = std::any_cast<std::pair<std::string, uint64_t>>(m_config->GetParams());
        timeval timeout;
        timeout.tv_sec = params.second / 1000;
        timeout.tv_usec = (params.second % 1000)* 1000;
        m_redis = redisConnectUnixWithTimeout(params.first.c_str(), timeout);
    }
        break;
    case RedisConnectionOption::kRedisConnectionWithFd:
    {
        int fd = std::any_cast<int>(m_config->GetParams());
        m_redis = redisConnectFd(fd);
    }
        break;
    default:
        break;
    }
    if(! m_redis || m_redis->err) {
        if(m_redis) {
            RedisLogError(m_logger->SpdLogger(), "[Redis connect to {}:{} failed, error is {}]", host.c_str(), port, m_redis->errstr);
            redisFree(m_redis);
            m_redis = nullptr;
            return false;
        } else {
            RedisLogError(m_logger->SpdLogger(), "[Redis connect to {}:{} failed]", host.c_str(), port);
            return false;           
        }
        return false;
    }
    RedisLogInfo(m_logger->SpdLogger(), "[Redis connect to {}:{}]", host.c_str(), port);
    redisReply *reply;
	// Authentication
	if (!password.empty()) {
        std::string query;
        if( version == 3 ) {
            query = "HELLO 3 ";
        } 
        query.reserve(query.length() + 6 + username.length() + password.length());
        query += ("AUTH " + username + " " + password);
        reply = static_cast<redisReply*>(redisCommand(m_redis, query.c_str()));
        if (!reply || reply->type == REDIS_REPLY_ERROR) {
            if (reply) {
                RedisLogError(m_logger->SpdLogger(), "[Authentication failure, error is {}]", reply->str);
                freeReplyObject(reply);
            } else {
                RedisLogError(m_logger->SpdLogger(), "[Authentication failure]");
            }
            return false;
        }
        freeReplyObject(reply);
        RedisLogInfo(m_logger->SpdLogger(), "[Authentication success]");
	}
	if( db_index != 0 ) {
        auto reply = SelectDB(db_index);
        if(reply.IsNull() || !reply.IsStatus()) return false;
    }
    return true;
}

bool RedisSession::DisConnect()
{
    if (m_redis) {
        redisFree(m_redis);
        m_redis = nullptr;
        return true;
    }
    return false;
}

RedisValue RedisSession::SelectDB(int32_t db_index)
{
    std::string cmd = std::format("SELECT {}", db_index);
    auto reply = RedisCommand(cmd);
    return RedisValue(reply);
}

RedisValue RedisSession::FlushDB()
{
    auto reply = RedisCommand("FLUSHDB");
    return RedisValue(reply);
}

RedisValue RedisSession::Switch(int version)
{
    std::string cmd = std::format("HELLO {}", version);
    auto reply = RedisCommand(cmd);
    return RedisValue(reply);
}

RedisValue RedisSession::Exist(const std::string &key)
{
    std::string cmd = std::format("EXISTS {}", key.c_str());
    auto reply = RedisCommand(cmd);
    return RedisValue(reply);
}

RedisValue RedisSession::Get(const std::string& key)
{
    std::string cmd = std::format("GET {}", key.c_str());
    auto reply = RedisCommand(cmd);
    return RedisValue(reply);
}

RedisValue RedisSession::Set(const std::string &key, const std::string &value)
{
    std::string cmd = std::format("SET {} {}", key.c_str(), value.c_str());
    auto reply = RedisCommand(cmd);
    return RedisValue(reply);
}

RedisValue RedisSession::Del(const std::string &key)
{
    std::string cmd = std::format("DEL {}", key.c_str());
    auto reply = RedisCommand(cmd);
    return RedisValue(reply);
}

RedisValue RedisSession::SetEx(const std::string &key, int64_t second, const std::string &value)
{
    std::string cmd = std::format("SETEX {} {} {}", key.c_str(), second, value.c_str());
    auto reply = RedisCommand(cmd);
    return RedisValue(reply);
}

RedisValue RedisSession::PSetEx(const std::string &key, int64_t milliseconds, const std::string &value)
{
    std::string cmd = std::format("PSETEX {} {} {}", key.c_str(), milliseconds, value.c_str());
    auto reply = RedisCommand(cmd);
    return RedisValue(reply);
}

RedisValue RedisSession::Incr(const std::string& key)
{
    std::string cmd = std::format("INCR {}", key.c_str());
    auto reply = RedisCommand(cmd);
    return RedisValue(reply);
}

RedisValue RedisSession::IncrBy(std::string key, int64_t value)
{
    std::string cmd = std::format("INCRBY {} {}", key.c_str(), value);
    auto reply = RedisCommand(cmd);
    return RedisValue(reply);
}

RedisValue RedisSession::Decr(const std::string& key)
{
    std::string cmd = std::format("DECR {}", key.c_str());
    auto reply = RedisCommand(cmd);
    return RedisValue(reply);
}

RedisValue RedisSession::HGet(const std::string& key, const std::string& field)
{
    std::string cmd = std::format("HGET {} {}", key.c_str(), field.c_str());
    auto reply = RedisCommand(cmd);
    return RedisValue(reply);
}

RedisValue RedisSession::HSet(const std::string &key, const std::string &field, const std::string &value)
{
    std::string cmd = std::format("HGET {} {}", key.c_str(), field.c_str());
    auto reply = RedisCommand(cmd);
    return RedisValue(reply);
}

RedisValue RedisSession::HGetAll(const std::string& key)
{
    std::string cmd = std::format("HGETALL {}", key.c_str());
    auto reply = RedisCommand(cmd);
    return RedisValue(reply);
}

RedisValue RedisSession::HIncrBy(const std::string& key, std::string field, int64_t value)
{
    std::string cmd = std::format("HINCRBY {} {} {}", key.c_str(), field.c_str(), value);
    auto reply = RedisCommand(cmd);
    return RedisValue(reply);
}

RedisValue RedisSession::LLen(const std::string& key)
{
    std::string cmd = std::format("LLEN {}", key.c_str());
    auto reply = RedisCommand(cmd);
    return RedisValue(reply);
}

RedisValue RedisSession::LRange(const std::string &key, int64_t start, int64_t end)
{
    std::string cmd = std::format("LRANGE {} {} {}", key.c_str(), start, end);
    auto reply = RedisCommand(cmd);
    return RedisValue(reply);
}

RedisValue RedisSession::LRem(const std::string &key, const std::string& value, int64_t count)
{
    std::string cmd = std::format("LREM {} {} {}", key.c_str(), count, value.c_str());
    auto reply = RedisCommand(cmd);
    return RedisValue(reply);
}

RedisValue RedisSession::SMembers(const std::string &key)
{
    std::string cmd = std::format("SMEMBERS {}", key.c_str());
    auto reply = RedisCommand(cmd);
    return RedisValue(reply);
}

RedisValue RedisSession::SMove(const std::string &source, const std::string &destination, const std::string &member)
{
    std::string cmd = std::format("SMOVE {} {} {}", source.c_str(), destination.c_str(), member.c_str());
    auto reply = RedisCommand(cmd);
    return RedisValue(reply);
}

RedisValue RedisSession::SCard(const std::string &key)
{
    std::string cmd = std::format("SCARD {}", key.c_str());
    auto reply = RedisCommand(cmd);
    return RedisValue(reply);
}

RedisValue RedisSession::ZRange(const std::string& key, uint32_t beg, uint32_t end)
{
    std::string cmd = std::format("ZRANGE {} {} {}", key.c_str(), beg, end);
    auto reply = RedisCommand(cmd);
    return RedisValue(reply);
}

RedisValue RedisSession::ZScore(const std::string &key, const std::string &member)
{
    std::string cmd = std::format("ZSCORE {} {}", key.c_str(), member.c_str());
    auto reply = RedisCommand(cmd);
    return RedisValue(reply);
}


redisReply *RedisSession::RedisCommand(const std::string &cmd)
{
    RedisLogInfo(m_logger->SpdLogger(), "[redisCommand is {}]", cmd);
    redisReply* reply = static_cast<redisReply*>(redisCommand(m_redis, cmd.c_str()));
    if (!reply) {
        RedisLogError(m_logger->SpdLogger(), "[redisCommand failed, error is {}]", m_redis->errstr);
        redisFree(m_redis);
        m_redis = nullptr;
        return nullptr;
    }
    return reply;
}

RedisSession::~RedisSession()
{
    DisConnect();
}

RedisAsyncSession::RedisAsyncSession(RedisConfig::ptr config, details::EventScheduler* scheduler, details::CoroutineScheduler* coroutine_scheduler)
    : m_config(config), m_logger(Logger::CreateStdoutLoggerMT("AsyncRedisLogger")), m_scheduler(scheduler), m_coScheduler(coroutine_scheduler), m_redis(nullptr)
{
    if(!m_scheduler) {
        if(EventSchedulerHolder::GetInstance()->GetSchedulerSize() == 0) {
            RedisLogError(m_logger->SpdLogger(), "No scheduler available, please start scheduler first.");
            throw std::runtime_error("No scheduler available, please start scheduler first.");
        }
        m_scheduler = EventSchedulerHolder::GetInstance()->GetScheduler();
    }
    m_action = std::make_unique<details::IOEventAction>(std::make_unique<details::RedisEvent>(this));
}

RedisAsyncSession::RedisAsyncSession(RedisConfig::ptr config, Logger::uptr logger, details::EventScheduler* scheduler, details::CoroutineScheduler* coroutine_scheduler)
    : m_config(config), m_logger(std::move(logger)), m_scheduler(scheduler), m_coScheduler(coroutine_scheduler), m_redis(nullptr)
{
    if(!m_scheduler) {
        if(EventSchedulerHolder::GetInstance()->GetSchedulerSize() == 0) {
            RedisLogError(m_logger->SpdLogger(), "No scheduler available, please start scheduler first.");
            throw std::runtime_error("No scheduler available, please start scheduler first.");
        }
        m_scheduler = EventSchedulerHolder::GetInstance()->GetScheduler();
    }
    m_action = std::make_unique<details::IOEventAction>(std::make_unique<details::RedisEvent>(this));
}

bool RedisAsyncSession::Connect(const std::string &ehost, int32_t port)
{
    std::string host = ehost;
    if( host == "localhost") {
        host = "127.0.0.1";
    }
    switch (m_config->GetConnectOption())
    {
    case RedisConnectionOption::kRedisConnectionWithNull:
        m_redis = redisAsyncConnect(host.c_str(), port);
        break;
    case RedisConnectionOption::kRedisConnectionWithBind:
    {
        std::string addr = std::any_cast<std::string>(m_config->GetParams());
        redisOptions options;
        REDIS_OPTIONS_SET_TCP(&options, host.c_str(), port);
        options.endpoint.tcp.source_addr = addr.c_str();
        m_redis = redisAsyncConnectWithOptions(&options);
    }
        break;
    case RedisConnectionOption::kRedisConnectionWithBindAndReuse:
    {
        std::string addr = std::any_cast<std::string>(m_config->GetParams());
        redisOptions options;
        REDIS_OPTIONS_SET_TCP(&options, host.c_str(), port);
        options.endpoint.tcp.source_addr = addr.c_str();
        options.options |= REDIS_OPT_REUSEADDR;
        m_redis = redisAsyncConnectWithOptions(&options);
    }
        break;
    case RedisConnectionOption::kRedisConnectionWithUnix:
    {
        std::string path = std::any_cast<std::string>(m_config->GetParams());
        m_redis = redisAsyncConnectUnix(path.c_str());
    }
        break;
    default:
        return false;
    }
    if(! m_redis || m_redis->err) {
        if(m_redis) {
            RedisLogError(m_logger->SpdLogger(), "[Redis connect to {}:{} failed, error is {}]", host.c_str(), port, m_redis->errstr);
            redisAsyncFree(m_redis);
            m_redis = nullptr;
            return false;
        } else {
            RedisLogError(m_logger->SpdLogger(), "[Redis connect to {}:{} failed]", host.c_str(), port);
            return false;           
        }
        return false;
    }
    HandleOption option({m_redis->c.fd});
    option.HandleNonBlock();
    m_redis->data = this;
    m_redis->c.flags |= REDIS_NO_AUTO_FREE_REPLIES;
    redisAsyncSetConnectCallback(m_redis, RedisConnectCallback);
    redisAsyncSetDisconnectCallback(m_redis, RedisDisconnectCallback);
    return true;
}

Coroutine<void> RedisAsyncSession::ReConnect(RoutineCtx routine)
{
    return RedisReconnect(routine, this);
}

int RedisAsyncSession::RedisAsyncCommand(const std::string &command)
{
    return redisAsyncCommand(m_redis, RedisCommandCallback, nullptr, command.c_str()) ;
}

RedisAsyncSession::~RedisAsyncSession()
{
    if(!m_redis) {
        redisAsyncFree(m_redis);
        m_redis = nullptr;
    }
}

void RedisAsyncSession::RedisConnectCallback(const redisAsyncContext *c, int status)
{
    auto redis = static_cast<RedisAsyncSession*>(c->data);
    auto awaiterbase = static_cast<details::RedisEvent*>(redis->m_action->GetBindEvent())->GetAwaiterBase();
    bool res = (status == REDIS_OK);
    static_cast<details::Awaiter<bool>*>(awaiterbase)->SetResult(std::move(res));
    if(status != REDIS_OK) {
        RedisLogError(redis->m_logger->SpdLogger(), "[Redis connect error: {}]", c->errstr);
    } else {
        RedisLogInfo(redis->m_logger->SpdLogger(), "[Redis connect success]");
    }
    static_cast<details::RedisEvent*>(redis->m_action->GetBindEvent())->ToResume();

}

void RedisAsyncSession::RedisDisconnectCallback(const redisAsyncContext *c, int status)
{
    auto redis = static_cast<RedisAsyncSession*>(c->data);
    RedisLogError(redis->m_logger->SpdLogger(), "[Redis disconnected]");
    if( redis->m_reconnect ) {
        RedisLogInfo(redis->m_logger->SpdLogger(), "[Redis reconnect.....]");
        redis->ReConnect(RoutineCtx::Create(redis->m_scheduler, redis->m_coScheduler));
    }
}

void RedisAsyncSession::RedisCommandCallback(redisAsyncContext *c, void *r, void *privdata)
{
    auto redis = static_cast<RedisAsyncSession*>(c->data);
    auto awaiterbase = static_cast<details::RedisEvent*>(redis->m_action->GetBindEvent())->GetAwaiterBase();
    RedisAsyncValue value(static_cast<redisReply*>(r));
    static_cast<details::Awaiter<RedisAsyncValue>*>(awaiterbase)->SetResult(std::move(value));
    static_cast<details::RedisEvent*>(redis->m_action->GetBindEvent())->ToResume();
}

Coroutine<void> RedisAsyncSession::RedisReconnect(RoutineCtx routine, RedisAsyncSession* session)
{
    bool success = co_await session->AsyncConnect<void>(session->m_host);
    if(success) {
        RedisLogInfo(session->m_logger->SpdLogger(), "RedisReconnect success");
    } else {
        RedisLogError(session->m_logger->SpdLogger(), "RedisReconnect error");
    }
    co_return;
}

}
