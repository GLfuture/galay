#ifndef GALAY_REDIS_TCC
#define GALAY_REDIS_TCC

#include "Redis.hpp"

namespace galay::redis 
{

template <KVPair... KV>
inline RedisValue RedisSession::MSet(KV... pairs)
{
    m_stream << "MSET";
    ((m_stream << " " << std::get<0>(pairs) << " " << std::get<1>(pairs)), ...);
    auto reply = RedisCommand(m_stream.str().c_str());
    m_stream.str("");
    return RedisValue(reply);
}

template<KeyType... Key>
inline RedisValue RedisSession::MGet(Key... keys)
{
    m_stream << "MGET";
    ((m_stream << " " << keys), ...);
    auto reply = RedisCommand(m_stream.str().c_str());
    m_stream.str("");
    return RedisValue(reply);
}

template <KeyType... Key>
inline RedisValue RedisSession::HDel(const std::string& key, Key... fields)
{
    m_stream << "HDEL " << key;
    ((m_stream << " " << fields), ...);
    auto reply = RedisCommand(m_stream.str().c_str());
    m_stream.str("");
    return RedisValue(reply);
}

template <KeyType... Key>
inline RedisValue RedisSession::HMGet(const std::string &key, Key... field)
{
    m_stream << "HMGET " << key;
    ((m_stream << " " << field), ...);
    auto reply = RedisCommand(m_stream.str().c_str());
    m_stream.str("");
    return RedisValue(reply);
}

template <KVPair... KV>
inline RedisValue RedisSession::HMSet(const std::string& key, KV... pairs)
{
    m_stream << "HMSET " << key;
    ((m_stream << " " << std::get<0>(pairs) << " " << std::get<1>(pairs)), ...);
    auto reply = RedisCommand(m_stream.str().c_str());
    m_stream.str("");
    return RedisValue(reply);
}

template <ValType... Val>
inline RedisValue RedisSession::LPush(const std::string& key, Val... values)
{
    m_stream << "LPUSH " << key;
    ((m_stream << " " << values), ...);
    auto reply = RedisCommand(m_stream.str().c_str());
    m_stream.str("");
    return RedisValue(reply);
}

template <ValType... Val>
inline RedisValue RedisSession::RPush(const std::string& key, Val... values)
{
    m_stream << "RPUSH " << key;
    ((m_stream << " " << values), ...);
    auto reply = RedisCommand(m_stream.str().c_str());
    m_stream.str("");
    return RedisValue(reply);
}

template <ValType... Val>
inline RedisValue RedisSession::SAdd(const std::string &key, Val... members)
{
    m_stream << "SADD " << key;
    ((m_stream << " " << members), ...);
    auto reply = RedisCommand(m_stream.str().c_str());
    m_stream.str("");
    return RedisValue(reply);
}

template <ValType... Val>
inline RedisValue RedisSession::SRem(const std::string &key, Val... members)
{
    m_stream << "SREM " << key;
    ((m_stream << " " << members), ...);
    auto reply = RedisCommand(m_stream.str().c_str());
    m_stream.str("");
    return RedisValue(reply);
}

template <KeyType... Key>
inline RedisValue RedisSession::SInter(Key... keys)
{
    m_stream << "SINTER";
    ((m_stream << " " << keys), ...);
    auto reply = RedisCommand(m_stream.str().c_str());
    m_stream.str("");
    return RedisValue(reply);
}

template <KeyType... Key>
inline RedisValue RedisSession::SUnion(Key... keys)
{
    m_stream << "SUNION";
    ((m_stream << " " << keys), ...);
    auto reply = RedisCommand(m_stream.str().c_str());
    m_stream.str("");
    return RedisValue(reply);
}

template <ScoreValType... KV>
inline RedisValue RedisSession::ZAdd(const std::string &key, KV... values)
{
    m_stream << "ZADD " << key;
    ((m_stream << " " << std::to_string(std::get<0>(values)) << " " << std::get<1>(values)), ...);
    auto reply = RedisCommand(m_stream.str().c_str());
    m_stream.str("");
    return RedisValue(reply);
}

template <KeyType... Key>
inline RedisValue RedisSession::ZRem(const std::string &key, Key... members)
{
    m_stream << "ZREM " << key;
    ((m_stream << " " << members), ...);
    auto reply = RedisCommand(m_stream.str().c_str());
    m_stream.str("");
    return RedisValue(reply);
}

template <typename... Args>
inline redisReply *RedisSession::RedisCommand(const std::string &cmd, Args... args)
{
    redisReply* reply = static_cast<redisReply*>(redisCommand(m_redis, cmd.c_str(), args...));
    if (!reply) {
        RedisLogError(m_logger->SpdLogger(), "[redisCommand failed: cmd is {}, error is {}]", cmd, m_redis->errstr);
        redisFree(m_redis);
        m_redis = nullptr;
        return nullptr;
    }
    return reply;
}

//Async
template <typename CoRtn>
inline AsyncResult<bool, CoRtn> RedisAsyncSession::AsyncConnect(THost host)
{
    m_host = host;
    if(!Connect(host.m_ip, host.m_port)) {
        return {false};
    }
    static_cast<details::RedisEvent*>(m_action->GetBindEvent())->ResetRedisWaitEventType(details::RedisWaitEventType_Write);
    return {m_action.get(), nullptr};
}

template <typename CoRtn>
inline AsyncResult<RedisAsyncValue, CoRtn> RedisAsyncSession::AsyncCommand(const std::string &command)
{
    if( RedisAsyncCommand(command) != REDIS_OK) {
        RedisAsyncValue value;
        return {std::move(value)};
    }
    static_cast<details::RedisEvent*>(m_action->GetBindEvent())->ResetRedisWaitEventType(details::RedisWaitEventType_Write);
    return {m_action.get(), nullptr};
}

}


#endif