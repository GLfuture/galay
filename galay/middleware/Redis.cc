#include "Redis.h"
#include <regex>

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
    m_replay = std::move(other.m_replay);
    other.m_replay = nullptr;
}

RedisValue &RedisValue::operator=(RedisValue &&other)
{
    m_replay = std::move(other.m_replay);
    other.m_replay = nullptr;
    return *this;
}

bool RedisValue::IsNull()
{
    return m_replay->type == REDIS_REPLY_NIL || m_replay == nullptr;
}

bool RedisValue::IsStatus()
{
    return m_replay->type == REDIS_REPLY_STATUS;
}

std::string RedisValue::ToStatus()
{
    return std::string(m_replay->str, m_replay->len);
}

bool RedisValue::IsError()
{
    return m_replay->type == REDIS_REPLY_ERROR;
}

std::string RedisValue::ToError()
{
    return std::string(m_replay->str, m_replay->len);
}

bool RedisValue::IsInteger()
{
    return m_replay->type == REDIS_REPLY_INTEGER;
}

int64_t RedisValue::ToInteger()
{
    return m_replay->integer;
}

bool RedisValue::IsString()
{
    return m_replay->type == REDIS_REPLY_STRING ;
}

std::string RedisValue::ToString()
{
    return std::string(m_replay->str, m_replay->len);
}

bool RedisValue::IsArray()
{
    return m_replay->type == REDIS_REPLY_ARRAY;
}

std::vector<RedisValue> RedisValue::ToArray()
{
    std::vector<RedisValue> result;
    result.reserve(m_replay->elements);
    for(size_t i = 0; i < m_replay->elements; ++i) {
        result.push_back(RedisValue(m_replay->element[i]));
    }
    return std::move(result);
}

bool RedisValue::IsDouble()
{
    return m_replay->type == REDIS_REPLY_DOUBLE;
}

double RedisValue::ToDouble()
{
    return strtod(m_replay->str, nullptr);
}

bool RedisValue::IsBool()
{
    return m_replay->type == REDIS_REPLY_BOOL;
}

bool RedisValue::ToBool()
{
    return m_replay->integer == 1;
}

bool RedisValue::IsMap()
{
    return m_replay->type == REDIS_REPLY_MAP;
}

std::map<std::string, RedisValue> RedisValue::ToMap()
{
    std::map<std::string, RedisValue> result;
    for(size_t i = 0; i < m_replay->elements; i += 2) {
        result.emplace(std::string(m_replay->element[i]->str, m_replay->element[i]->len), RedisValue(m_replay->element[i + 1]));
    }
    return std::move(result);
}

bool RedisValue::IsSet()
{
    return m_replay->type == REDIS_REPLY_SET;
}

std::vector<RedisValue> RedisValue::ToSet()
{
    std::vector<RedisValue> result;
    result.reserve(m_replay->elements);
    for(size_t i = 0; i < m_replay->elements; ++i) {
        result.push_back(RedisValue(m_replay->element[i]));
    }
    return std::vector<RedisValue>();
}

bool RedisValue::IsAttr()
{
    return m_replay->type == REDIS_REPLY_ATTR;
}

bool RedisValue::IsPush()
{
    return m_replay->type == REDIS_REPLY_PUSH;
}

std::vector<RedisValue> RedisValue::ToPush()
{
    std::vector<RedisValue> result;
    result.reserve(m_replay->elements);
    for(size_t i = 0; i < m_replay->elements; ++i) {
        result.push_back(RedisValue(m_replay->element[i]));
    }
    return std::move(result);
}

bool RedisValue::IsBigNumber()
{
    return m_replay->type == REDIS_REPLY_BIGNUM;
}

std::string RedisValue::ToBigNumber()
{
    return std::string(m_replay->str, m_replay->len);
}

bool RedisValue::IsVerb()
{
    return m_replay->type == REDIS_REPLY_VERB;
}

std::string RedisValue::ToVerb()
{
    return std::string(m_replay->str, m_replay->len);
}

RedisValue::~RedisValue()
{
    if(m_replay) {
        freeReplyObject(m_replay);
    }
}


RedisSession::RedisSession(RedisConfig::ptr config)
    : m_config(config)
{
}

// redis://[password@]host[:port][/db_index]
bool RedisSession::Connect(const std::string &url)
{
    std::regex pattern(R"(^redis:\/\/(([^:]+):)?([^@]+)@([^:\/]+)(?::(\d+))?(?:\/(\d+))?)");
    std::smatch matches;
    std::string username, password, host;
    int32_t port = 6379, db_index = 0;
    if (std::regex_match(url, matches, pattern)) {
        if (matches.size() > 2 && !matches[2].str().empty()) {
            username = matches[2];
        }
        if (matches.size() > 3 && !matches[3].str().empty()) {
            password = matches[3];
        }
        if (matches.size() > 4 && !matches[4].str().empty()) {
            host = matches[4];
        } else {
            LogError("[Redis host is invalid]");
            return false;
        }
        if (matches.size() > 5 && !matches[5].str().empty()) {
            try
            {
                port = std::stoi(matches[5]);
            }
            catch(const std::exception& e)
            {
                LogError("[Redis url is invalid]");
                return false;
            }
        }
        if (matches.size() > 6 && !matches[6].str().empty()) {
            try
            {
                db_index = std::stoi(matches[6]);
            }
            catch(const std::exception& e)
            {
                LogError("[Redis url is invalid]");
                return false;
            }
        }
    
    } else {
        LogError("[Redis url is invalid]");
        return false;
    }
    return Connect(host, port, username, password, db_index);
}

bool RedisSession::Connect(const std::string &host, uint32_t port, const std::string& username, const std::string &password)
{
    return Connect(host, port, username, password, 0);
}

bool RedisSession::Connect(const std::string &host, uint32_t port, const std::string& username, const std::string &password, uint32_t db_index)
{
    return Connect(host, port, username, password, db_index, 2);
}

bool RedisSession::Connect(const std::string &host, uint32_t port, const std::string& username, const std::string &password, uint32_t db_index, int version)
{
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
            LogError("[Redis connect to {}:{} failed, error is {}]", host.c_str(), port, m_redis->errstr);
            redisFree(m_redis);
            m_redis = nullptr;
            return false;
        } else {
            LogError("[Redis connect to {}:{} failed]", host.c_str(), port);
            return false;           
        }
        return false;
    }
    LogInfo("[Redis connect to {}:{}]", host.c_str(), port);
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
                LogError("Authentication failure, error is {}", reply->str);
                freeReplyObject(reply);
            } else {
                LogError("Authentication failure");
            }
            return false;
        }
        freeReplyObject(reply);
        LogInfo("Authentication success");
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

RedisValue RedisSession::SelectDB(uint32_t db_index)
{
    auto reply = RedisCommand("SELECT %d", db_index);
    return RedisValue(reply);
}

RedisValue RedisSession::FlushDB()
{
    auto reply = RedisCommand("FLUSHDB");
    return RedisValue(reply);
}

RedisValue RedisSession::Switch(int version)
{
    auto reply = RedisCommand("HELLO %d", version);
    return RedisValue(reply);
}

RedisValue RedisSession::Exist(const std::string &key)
{
    auto reply = RedisCommand("EXISTS %s", key.c_str());
    return RedisValue(reply);
}

RedisValue RedisSession::Get(const std::string& key)
{
    auto reply = RedisCommand("GET %s", key.c_str());
    return RedisValue(reply);
}

RedisValue RedisSession::Set(const std::string &key, const std::string &value)
{
    auto reply = RedisCommand("SET %s %s", key.c_str(), value.c_str());
    return RedisValue(reply);
}

RedisValue RedisSession::Del(const std::string &key)
{
    auto reply = RedisCommand("DEL %s", key.c_str());
    return RedisValue(reply);
}

RedisValue RedisSession::SetEx(const std::string &key, int64_t second, const std::string &value)
{
    auto reply = RedisCommand("SETEX %s %lld %s", key.c_str(), second, value.c_str());
    return RedisValue(reply);
}

RedisValue RedisSession::PSetEx(const std::string &key, int64_t milliseconds, const std::string &value)
{
    auto reply = RedisCommand("PSETEX %s %lld %s", key.c_str(), milliseconds, value.c_str());
    return RedisValue(reply);
}

RedisValue RedisSession::Incr(const std::string& key)
{
    auto reply = RedisCommand("INCR %s", key.c_str());
    return RedisValue(reply);
}

RedisValue RedisSession::IncrBy(std::string key, int64_t value)
{
    auto reply = RedisCommand("INCRBY %s %lld", key.c_str(), value);
    return RedisValue(reply);
}

RedisValue RedisSession::Decr(const std::string& key)
{
    auto reply = RedisCommand("DECR %s", key.c_str());
    return RedisValue(reply);
}

RedisValue RedisSession::HGet(const std::string& key, const std::string& field)
{
    auto reply = RedisCommand("HGET %s %s", key.c_str(), field.c_str());
    return RedisValue(reply);
}

RedisValue RedisSession::HSet(const std::string &key, const std::string &field, const std::string &value)
{
    auto reply = RedisCommand("HSET %s %s %s", key.c_str(), field.c_str(), value.c_str());
    return RedisValue(reply);
}

RedisValue RedisSession::HGetAll(const std::string& key)
{
    auto reply = RedisCommand("HGETALL %s", key.c_str());
    return RedisValue(reply);
}

RedisValue RedisSession::HIncrBy(const std::string& key, std::string field, int64_t value)
{
    auto reply = RedisCommand("HINCRBY %s %s %lld", key.c_str(), field.c_str(), value);
    return RedisValue(reply);
}

RedisValue RedisSession::LLen(const std::string& key)
{
    auto reply = RedisCommand("LLEN %s", key.c_str());
    return RedisValue(reply);
}

RedisValue RedisSession::LRange(const std::string &key, int64_t start, int64_t end)
{
    auto reply = RedisCommand("LRANGE %s %lld %lld", key.c_str(), start, end);
    return RedisValue(reply);
}

RedisValue RedisSession::LRem(const std::string &key, const std::string& value, int64_t count)
{
    auto reply = RedisCommand("LREM %s %lld %s", key.c_str(), count, value.c_str());
    return RedisValue(reply);
}

RedisValue RedisSession::SMembers(const std::string &key)
{
    auto reply = RedisCommand("SMEMBERS %s", key.c_str());
    return RedisValue(reply);
}

RedisValue RedisSession::SMove(const std::string &source, const std::string &destination, const std::string &member)
{
    auto reply = RedisCommand("SMOVE %s %s %s", source.c_str(), destination.c_str(), member.c_str());
    return RedisValue(reply);
}

RedisValue RedisSession::ZRange(const std::string& key, uint32_t beg, uint32_t end)
{
    auto reply = RedisCommand("ZRANGE %s %u %u", key.c_str(), beg, end);
    return RedisValue(reply);
}

RedisValue RedisSession::ZScore(const std::string &key, const std::string &member)
{
    auto reply = RedisCommand("ZSCORE %s %s", key.c_str(), member.c_str());
    return RedisValue(reply);
}

RedisSession::~RedisSession()
{
    DisConnect();
}

AsyncRedisSession::AsyncRedisSession(RedisConfig::ptr config)
{
    
}

}