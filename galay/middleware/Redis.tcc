#ifndef GALAY_REDIS_TCC
#define GALAY_REDIS_TCC

#include "Redis.h"

namespace galay::redis 
{

template <KVPair... KV>
inline RedisValue RedisSession::MSet(KV... pairs)
{
    std::string command = "MSET";
    ((command = command + " " + std::get<0>(pairs) + " " + std::get<1>(pairs)), ...);
    auto reply = RedisCommand(command.c_str());
    return RedisValue(reply);
}

template<KeyType... Key>
inline RedisValue RedisSession::MGet(Key... keys)
{
    std::string command = "MGET";
    ((command = command + " " + keys), ...);
    auto reply = RedisCommand(command.c_str());
    return RedisValue(reply);
}

template <KeyType... Key>
inline RedisValue RedisSession::HDel(const std::string& key, Key... fields)
{
    std::string commad = "HDEL ";
    commad += key;
    ((commad = commad + " " + fields), ...);
    auto reply = RedisCommand(commad.c_str());
    return RedisValue(reply);
}

template <KeyType... Key>
inline RedisValue RedisSession::HMGet(const std::string &key, Key... field)
{
    std::string command = "HMGET ";
    command += key;
    ((command = command + " " + field), ...);
    auto reply = RedisCommand(command.c_str());
    return RedisValue(reply);
}

template <KVPair... KV>
inline RedisValue RedisSession::HMSet(const std::string& key, KV... pairs)
{
    std::string command = "HMSET ";
    command += key;
    ((command = command + " " + std::get<0>(pairs) + " " + std::get<1>(pairs)), ...);
    auto reply = RedisCommand(command.c_str());
    return RedisValue(reply);
}

template <ValType... Val>
inline RedisValue RedisSession::LPush(const std::string& key, Val... values)
{
    std::string command = "LPUSH ";
    command += key;
    ((command = command + " " + values), ...);
    auto reply = RedisCommand(command.c_str());
    return RedisValue(reply);
}

template <ValType... Val>
inline RedisValue RedisSession::RPush(const std::string& key, Val... values)
{
    std::string command = "RPUSH ";
    command += key;
    ((command = command + " " + values), ...);
    auto reply = RedisCommand(command.c_str());
    return RedisValue(reply);
}

template <ValType... Val>
inline RedisValue RedisSession::SAdd(const std::string &key, Val... members)
{
    std::string command = "SADD ";
    command += key;
    ((command = command + " " + members), ...);
    auto reply = RedisCommand(command.c_str());
    return RedisValue(reply);
}

template <ValType... Val>
inline RedisValue RedisSession::SRem(const std::string &key, Val... members)
{
    std::string command = "SREM ";
    command += key;
    ((command = command + " " + members), ...);
    auto reply = RedisCommand(command.c_str());
    return RedisValue(reply);
}

template <KeyType... Key>
inline RedisValue RedisSession::SInter(Key... keys)
{
    std::string command = "SINTER";
    ((command = command + " " + keys), ...);
    auto reply = RedisCommand(command.c_str());
    return RedisValue(reply);
}

template <KeyType... Key>
inline RedisValue RedisSession::SUnion(Key... keys)
{
    std::string command = "SUNION";
    ((command = command + " " + keys), ...);
    auto reply = RedisCommand(command.c_str());
    return RedisValue(reply);
}

template <ScoreValType... KV>
inline RedisValue RedisSession::ZAdd(const std::string &key, KV... values)
{
    std::string command = "ZADD ";
    command += key;
    ((command = command + " " + std::to_string(std::get<0>(values)) + " " + std::get<1>(values)), ...);
    auto reply = RedisCommand(command.c_str());
    return RedisValue(reply);
}

template <KeyType... Key>
inline RedisValue RedisSession::ZRem(const std::string &key, Key... members)
{
    std::string command = "ZREM ";
    command += key;
    ((command = command + " " + members), ...);
    auto reply = RedisCommand(command.c_str());
    return RedisValue(reply);
}

template <typename... Args>
inline redisReply *RedisSession::RedisCommand(const std::string &cmd, Args... args)
{
    std::string query = "";
    RedisLogInfo(m_logger->SpdLogger(), "[redisCommand: cmd is {}]", cmd);
    redisReply* reply = static_cast<redisReply*>(redisCommand(m_redis, cmd.c_str(), args...));
    if (!reply) {
        RedisLogError(m_logger->SpdLogger(), "[redisCommand failed: cmd is {}, error is {}]", cmd, m_redis->errstr);
        redisFree(m_redis);
        m_redis = nullptr;
        return nullptr;
    }
    return reply;
}
}


#endif