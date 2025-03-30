#ifndef GALAY_REDIS_HPP
#define GALAY_REDIS_HPP

#include <hiredis/async.h>
#include <any>
#include <map>
#include <string>
#include "galay/kernel/ExternApi.hpp"
#include "galay/kernel/Log.h"

namespace galay::redis
{
	class RedisSession;
	class RedisAsyncSession;
}

namespace galay::details
{

enum RedisWaitEventType
{
	RedisWaitEventType_None = 0,
	RedisWaitEventType_Read, //读完需要重新触发读事件以接收sever那边的disconnect事件
	RedisWaitEventType_Write,
};

class RedisEvent: public WaitEvent
{
public:
	RedisEvent(redis::RedisAsyncSession* redis);
	virtual bool SetEventEngine(EventEngine* engine) override;
	virtual EventType GetEventType() override;
	virtual GHandle GetHandle() override;
	virtual void HandleEvent(EventEngine* engine) override;
	virtual std::string Name() override;
	virtual bool OnWaitPrepare(CoroutineBase::wptr co, void* ctx) override;

	AwaiterBase* GetAwaiterBase();
	void ToResume();
	void ResetRedisWaitEventType(RedisWaitEventType type) { m_redisWaitType = type; }
	~RedisEvent();
private:
	bool OnWaitReadPrepare(CoroutineBase::wptr co, void* ctx);
	bool OnWaitWritePrepare(CoroutineBase::wptr co, void* ctx);

	bool HandleRedisRead(EventEngine* engine);
	bool HandleRedisWrite(EventEngine* engine);
private:
	void* m_ctx;
	CoroutineBase::wptr m_waitco;
	redis::RedisAsyncSession* m_redis;
	RedisWaitEventType m_redisWaitType = RedisWaitEventType_None;
};


}

namespace galay::redis 
{

#define RedisLogTrace(logger, ...)       SPDLOG_LOGGER_TRACE(logger, __VA_ARGS__)
#define RedisLogDebug(logger, ...)       SPDLOG_LOGGER_DEBUG(logger, __VA_ARGS__)
#define RedisLogInfo(logger, ...)        SPDLOG_LOGGER_INFO(logger, __VA_ARGS__)
#define RedisLogWarn(logger, ...)        SPDLOG_LOGGER_WARN(logger, __VA_ARGS__)
#define RedisLogError(logger, ...)       SPDLOG_LOGGER_ERROR(logger, __VA_ARGS__)
#define RedisLogCritical(logger, ...)    SPDLOG_LOGGER_CRITICAL(logger, __VA_ARGS__)



enum class RedisConnectionOption {
    kRedisConnectionWithNull,
    kRedisConnectionWithTimeout,				//设置超时时间,只适用于RedisSession，异步设置无效
    kRedisConnectionWithBind,                   //绑定本地地址
    kRedisConnectionWithBindAndReuse,           //绑定本地地址并设置SO_REUSEADDR
    kRedisConnectionWithUnix,                   //使用unix域套接字
    kRedisConnectionWithUnixAndTimeout,			//设置超时时间,只适用于RedisSession，异步设置无效
    kRedisConnectionWithFd,						//使用文件描述符,异步设置无效
};


class RedisConfig 
{
public: 
    using ptr = std::shared_ptr<RedisConfig>;

	static RedisConfig::ptr CreateConfig();

    void ConnectWithTimeout(uint64_t timeout);
    void ConnectWithBind(const std::string& addr);
    void ConnectWithBindAndReuse(const std::string& addr);
    void ConnectWithUnix(const std::string& path);
    void ConnectWithUnixAndTimeout(const std::string& path, uint64_t timeout);
    void ConnectWithFd(int fd);

    RedisConnectionOption& GetConnectOption();
    std::any& GetParams();

private:
    std::any m_params;
    RedisConnectionOption m_connection_option = RedisConnectionOption::kRedisConnectionWithNull;
};

class RedisValue 
{
public:
	RedisValue(): m_replay(nullptr) {}
    RedisValue(redisReply* reply): m_replay(reply) {}
	RedisValue(RedisValue&& other);
	RedisValue& operator=(RedisValue&& other);
	RedisValue(const RedisValue&) = delete;
	RedisValue& operator=(const RedisValue&) = delete;
    // Resp2
    bool IsNull();
    bool IsStatus();
    std::string ToStatus();
    bool IsError();
    std::string ToError();
    bool IsInteger();
    int64_t ToInteger();
    bool IsString();
    std::string ToString();
    bool IsArray();
    std::vector<RedisValue> ToArray();
    
    //Resp3
    bool IsDouble();
    double ToDouble();
    bool IsBool();
    bool ToBool();
    bool IsMap();
    std::map<std::string, RedisValue> ToMap();
    bool IsSet();
    std::vector<RedisValue> ToSet();
    bool IsAttr();
    bool IsPush();
    std::vector<RedisValue> ToPush();
    bool IsBigNumber();
    std::string ToBigNumber();
    //不转义字符串
    bool IsVerb();
    std::string ToVerb();

    virtual ~RedisValue();
protected:
    redisReply* m_replay;
};

class RedisAsyncValue: public RedisValue
{
public:
	RedisAsyncValue(): RedisValue(nullptr) {}
    RedisAsyncValue(redisReply* reply): RedisValue(reply) {}
	RedisAsyncValue(RedisAsyncValue&& other);
	RedisAsyncValue& operator=(RedisAsyncValue&& other);
	RedisAsyncValue(const RedisAsyncValue&) = delete;
	RedisAsyncValue& operator=(const RedisAsyncValue&) = delete;
	~RedisAsyncValue() = default;
private:
};

template <typename T>
concept KVPair = requires(T type)
{
	std::is_same_v<T, std::pair<std::string, std::string>>;
};

template <typename T>
concept KeyType = requires(T type)
{
	std::is_same_v<T, std::string>;
};

template <typename T>
concept ValType = requires(T type)
{
	std::is_same_v<T, std::string>;
};

template <typename T>
concept ScoreValType = requires(T type)
{
	std::is_same_v<T, std::pair<double, std::string>>;
};

class RedisSession 
{
public:

    RedisSession(RedisConfig::ptr config);
	RedisSession(RedisConfig::ptr config, Logger::ptr logger);
    bool Connect(const std::string& url);
    bool Connect(const std::string& host, int32_t port, const std::string& username, const std::string& password);
    bool Connect(const std::string& host, int32_t port, const std::string& username, const std::string& password, int32_t db_index);
    bool Connect(const std::string& host, int32_t port, const std::string& username, const std::string& password, int32_t db_index, int version);
    
	bool DisConnect();
	/*
	* return : status
	*/
    RedisValue SelectDB(int32_t db_index);
    /*
	* return : status
	*/
	RedisValue FlushDB();
    /*
	* role: 切换版本
	* RESP2 return: array
	* RESP3 return: map
	*/
    RedisValue Switch(int version);
	/*
	* return: integer(1, exist, 0, not exist)
	*/
	RedisValue Exist(const std::string &key);
	/*
	* return: RedisValue
	*/
    RedisValue Get(const std::string& key);
	/*
	* return: status
	*/
	RedisValue Set(const std::string& key, const std::string& value);
	/*
	* return: status
	*/
	template <KVPair... KV>
	RedisValue MSet(KV... pairs);
	/*
	* return: RedisValue array
	*/
	template<KeyType... Key>
	RedisValue MGet(Key... keys);
	/*
	* return: integer
	*/
	RedisValue Del(const std::string &key);
	/*
	* return: status
	*/
	RedisValue SetEx(const std::string& key, int64_t seconds, const std::string& value);
	/*
	* return: status
	*/
	RedisValue PSetEx(const std::string& key, int64_t milliseconds, const std::string& value); 
	/*
	* return: integer
	*/
	RedisValue Incr(const std::string& key);
	/*
	* return: integer
	*/
	RedisValue IncrBy(std::string key, int64_t value);
	/*
	* return: integer
	*/
	RedisValue Decr(const std::string& key);
	/*
	* return: RedisValue
	*/
	RedisValue HGet(const std::string& key, const std::string& field);
	/*
	* return: status
	*/
	RedisValue HSet(const std::string& key, const std::string& field, const std::string& value);
	/*
	* return: integer
	*/
	template <KeyType... Key>
	RedisValue HDel(const std::string& key, Key... fields);
	/*
	* return: integer
	*/
	template <KeyType... Key>
	RedisValue HMGet(const std::string& key, Key... field);
	/*
	* return: integer
	*/
	template<KVPair... KV>
	RedisValue HMSet(const std::string& key, KV... fields);
	/*
	* return: RedisValue map or array
	*/
	RedisValue HGetAll(const std::string& key);
	/*
	* return: integer
	*/
	RedisValue HIncrBy(const std::string& key, std::string field, int64_t value);
	/*
	* return: integer
	*/
	template <ValType... Val>
	RedisValue LPush(const std::string& key, Val... values);
	/*
	* return: integer
	*/
	template <ValType... Val>
	RedisValue RPush(const std::string& key, Val... values);
	/*
	* return: integer
	*/
	RedisValue LLen(const std::string& key);
	/*
	* return: RedisValue array
	*/
	RedisValue LRange(const std::string& key, int64_t start, int64_t end);
	/*
	* return: integer
	*/
	RedisValue LRem(const std::string& key, const std::string& value, int64_t count);
	/*
	* return: integer
	*/
	template <ValType... Val>
	RedisValue SAdd(const std::string& key, Val... members);
	/*
	* return: RedisValue array
	*/
	RedisValue SMembers(const std::string& key);
	/*
	* return: integer
	*/
	template <ValType... Val>
	RedisValue SRem(const std::string& key, Val... members);
	/*
	* return: RedisValue array or set
	*/
	template<KeyType... Key>
	RedisValue SInter(Key... keys);
	/*
	* return: RedisValue array or set
	*/
	template<KeyType... Key>
	RedisValue SUnion(Key... keys);
	/*
	* return: integer
	*/
	RedisValue SMove(const std::string& source, const std::string& destination, const std::string& member);
	/*
	* return: integer
	*/
	template <ScoreValType... KV>
	RedisValue ZAdd(const std::string& key, KV... values);
	/*
	* return: RedisValue array
	*/
	RedisValue ZRange(const std::string& key, uint32_t beg, uint32_t end);
	/*
	* return: string or double
	*/
	RedisValue ZScore(const std::string& key, const std::string& member);
	
	template <KeyType... Key>
	RedisValue ZRem(const std::string& key, Key... members);


    template<typename... Args>
    redisReply* RedisCommand(const std::string& cmd, Args... args);

    ~RedisSession();
private:
	Logger::ptr m_logger;
	std::ostringstream m_stream;
    redisContext* m_redis;
    RedisConfig::ptr m_config;
};

class RedisAsyncSession 
{
	friend class details::RedisEvent;
public:
	RedisAsyncSession(RedisConfig::ptr config, details::EventScheduler* scheduler = nullptr, details::CoroutineScheduler* coroutine_scheduler = nullptr);
	RedisAsyncSession(RedisConfig::ptr config, Logger::ptr logger, details::EventScheduler* scheduler = nullptr, details::CoroutineScheduler* coroutine_scheduler = nullptr);	
	template<typename CoRtn>
	AsyncResult<bool, CoRtn> AsyncConnect(THost host);

	template<typename CoRtn>
	AsyncResult<RedisAsyncValue, CoRtn> AsyncCommand(const std::string& command);

	void StartReConnect();

	~RedisAsyncSession();
private:
	static void RedisConnectCallback(const redisAsyncContext *c, int status);
	static void RedisDisconnectCallback(const redisAsyncContext *c, int status);
	static void RedisCommandCallback(redisAsyncContext *c, void *r, void *privdata);

	static Coroutine<void> RedisReconnect(RoutineCtx routine, RedisAsyncSession* session);

	//关闭hiredis自动释放reply
	bool Connect(const std::string& host, int32_t port);
	Coroutine<void> ReConnect(RoutineCtx routine);

	int RedisAsyncCommand(const std::string& command);
private:
	THost m_host;
	bool m_reconnect = false;
	Logger::ptr m_logger;
	redisAsyncContext* m_redis;
	RedisConfig::ptr m_config;
	details::IOEventAction::uptr m_action;
	details::EventScheduler* m_scheduler;
	details::CoroutineScheduler* m_coScheduler;
};


}

#include "Redis.tcc"

#endif