#ifndef GALAY_MYSQL_HPP
#define GALAY_MYSQL_HPP

#include <mysql/mysql.h>
#include <string>
#include <vector>
#include <typeinfo>
#include <cxxabi.h>
#include <memory>
#include <string.h>
#include <concurrentqueue/moodycamel/concurrentqueue.h>
#include "galay/kernel/Awaiter.h"
#include "galay/utils/Pool.hpp"

namespace galay::mysql
{

#define MysqlLogTrace(logger, ...)       SPDLOG_LOGGER_TRACE(logger, __VA_ARGS__)
#define MysqlLogDebug(logger, ...)       SPDLOG_LOGGER_DEBUG(logger, __VA_ARGS__)
#define MysqlLogInfo(logger, ...)        SPDLOG_LOGGER_INFO(logger, __VA_ARGS__)
#define MysqlLogWarn(logger, ...)        SPDLOG_LOGGER_WARN(logger, __VA_ARGS__)
#define MysqlLogError(logger, ...)       SPDLOG_LOGGER_ERROR(logger, __VA_ARGS__)
#define MysqlLogCritical(logger, ...)    SPDLOG_LOGGER_CRITICAL(logger, __VA_ARGS__)

enum class MysqlOrderBy : uint8_t {
    ASC,
    DESC
};

typedef std::pair<std::string, MysqlOrderBy> FieldOrderByPair;
typedef std::pair<std::string, std::string> FieldValuePair;

class MySqlException : public std::exception
{
public:
    MySqlException(std::string message);
    virtual const char *what() const noexcept override;
private:
    std::string m_message;
};

class MysqlStmtExecutor
{
public:
    using ptr = std::shared_ptr<MysqlStmtExecutor>;
    MysqlStmtExecutor(MYSQL_STMT* stmt, Logger::ptr logger);
    bool Prepare(const std::string& ParamQuery);
    bool BindParam(MYSQL_BIND* params);
    //MysqlStmtValue 类型为blob或string 需要调用
    bool StringToParam(const std::string& data, unsigned int index);
    bool BindResult(MYSQL_BIND* result);
    bool Execute();
    bool GetARow(MYSQL_BIND* result);

    bool Close();
    std::string GetLastError();
    ~MysqlStmtExecutor();
private:
    bool StoreResult();
private:
    MYSQL_STMT* m_stmt;
    bool m_storeResult;
    Logger::ptr m_logger;
};

class MysqlSession;
class AsyncMysqlSession;

struct MysqlSSLSettings
{
    std::string key;
    std::string cert;
    std::string ca;
    std::string capath;     //option
    std::string cipher;     //option
};

class MysqlConfig
{
    friend class MysqlSession;
    friend class AsyncMysqlSession;
public:
    using ptr = std::shared_ptr<MysqlConfig>;
    static MysqlConfig::ptr CreateConfig(); 
    MysqlConfig& WithCharset(const std::string& charset);
    MysqlConfig& WithConnectTimeout(uint32_t timeout);
    MysqlConfig& WithReadTimeout(uint32_t timeout);
    MysqlConfig& WithWriteTimeout(uint32_t timeout);
    MysqlConfig& WithCompress(bool enable);                 //启用压缩，增加cpu开销，减少网络开销
    MysqlConfig& WithSSL(MysqlSSLSettings setting);
private:
    bool m_enableSSL = false;
    MysqlSSLSettings m_ssl_setting;
    bool m_compress = false;
    std::string m_charset = "utf8mb4";
    uint32_t m_connectTimeout = -1;
    uint32_t m_readTimeout = -1;
    uint32_t m_writeTimeout = -1;
};

const uint32_t MYSQL_FIELD_RESTRAINT_NOT_NULL = 1;
const uint32_t MYSQL_FIELD_RESTRAINT_UNIQUE = 2;
const uint32_t MYSQL_FIELD_RESTRAINT_PRIMARY_KEY = 4;
const uint32_t MYSQL_FIELD_RESTRAINT_AUTO_INCREMENT = 8;
const uint32_t MYSQL_FIELD_RESTRAINT_FOREIGN_KEY = 16;
const uint32_t MYSQL_FIELD_RESTRAINT_DEFAULT = 32;
const uint32_t MYSQL_FIELD_RESTRAINT_CHECK = 64;
const uint32_t MYSQL_FIELD_RESTRAINT_COLLATE = 128;
const uint32_t MYSQL_FIELD_RESTRAINT_COMMENT = 256;
const uint32_t MYSQL_FIELD_RESTRAINT_UNSIGNED = 512;
const uint32_t MYSQL_FIELD_RESTRAINT_ZEROFILL = 1024;
const uint32_t MYSQL_FIELD_RESTRAINT_ON_UPDATE_CURRENT_TIMESTAMP = 2048; //记录当前时间戳

class MysqlTable;

class MysqlField 
{
    friend class MysqlTable;
public:
    MysqlField& Name(const std::string& name);
    MysqlField& Type(const std::string& type);
    MysqlField& AutoIncrement();
    MysqlField& NotNull();
    MysqlField& Unique();
    MysqlField& Default(const std::string& defaultValue);
    MysqlField& Comment(const std::string& comment);
    std::string ToString() const;
private:
    std::string m_name;
    std::ostringstream m_stream;
};

class MysqlTable
{
public:
    MysqlTable(const std::string& name);
    MysqlField& PrimaryKey();
    /* such as user(uid) */
    MysqlField& ForeignKey(const std::string& name);
    MysqlField& CreateUpdateTimeStampField();
    MysqlTable& ExtraAction(const std::string& action);
    std::string ToString() const;
private:
    std::string m_name;
    std::vector<MysqlField> m_primary_keys;
    std::vector<std::pair<MysqlField, std::string>> m_foreign_keys;
    std::vector<MysqlField> m_other_fields;
    std::string m_extra_action = "ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";
};

template <typename T>
concept FieldNameType = requires () {
    std::is_same_v<T, std::string>;
};

template <typename T>
concept TableNameType = requires () {
    std::is_same_v<T, std::string>;
};


std::string OrderByToString(MysqlOrderBy orderby);

template <typename T>
concept OrderByType = requires (T type) {
    std::is_convertible_v<decltype(type.first), std::string>;
    std::is_convertible_v<decltype(type.second), MysqlOrderBy>;
};

template <typename T>
concept FiledValueType = requires (T type) {
    std::is_convertible_v<decltype(type.first), std::string>;
    std::is_convertible_v<decltype(type.second), std::string>;
};

class MysqlQuery
{
public:
    std::string ToString();
protected:
    std::ostringstream m_stream;
};

class MysqlSelectQuery: public MysqlQuery
{
public:
    using ptr = std::shared_ptr<MysqlSelectQuery>;
    MysqlSelectQuery() = default;
    template <FieldNameType ...Field>
    MysqlSelectQuery& Fields(Field... fields);
    template <TableNameType ...Table>
    MysqlSelectQuery& From(Table... tables);
    MysqlSelectQuery& InnerJoinOn(const std::string& table, const std::string& condition);
    MysqlSelectQuery& LeftJoinOn(const std::string& table, const std::string& condition);
    MysqlSelectQuery& RightJoinOn(const std::string& table, const std::string& condition);
    MysqlSelectQuery& FullJoinOn(const std::string& table, const std::string& condition);
    MysqlSelectQuery& CrossJoin(const std::string& table);
    MysqlSelectQuery& Union();
    MysqlSelectQuery& Where(const std::string& condition);
    template <FieldNameType ...Field>
    MysqlSelectQuery& GroupBy(Field... fields);
    MysqlSelectQuery& Having(const std::string& condition);
    template <OrderByType ...OrderType>
    MysqlSelectQuery& OrderBy(OrderType... orders);
    MysqlSelectQuery& Limit(uint32_t limit);
};

class MysqlInsertQuery: public MysqlQuery
{
public:
    MysqlInsertQuery() = default;
    MysqlInsertQuery& Table(const std::string& table);
    template<FiledValueType... FieldsValue>
    MysqlInsertQuery& FieldValues(FieldsValue... values);
};

class MysqlUpdateQuery: public MysqlQuery
{
public:
    MysqlUpdateQuery() = default;
    MysqlUpdateQuery& Table(const std::string& table);
    template<FiledValueType... FieldsValue>
    MysqlUpdateQuery& FieldValues(FieldsValue... values);
    MysqlUpdateQuery& Where(const std::string& condition);
};

class MysqlDeleteQuery: public MysqlQuery
{
public:
    MysqlDeleteQuery() = default;
    MysqlDeleteQuery& Table(const std::string& table);
    MysqlDeleteQuery& Where(const std::string& condition);
};

class MysqlAlterQuery: public MysqlQuery
{
public:
    MysqlAlterQuery() = default;
    MysqlAlterQuery& Table(const std::string& table);
    MysqlAlterQuery& AddColumn(MysqlField& field);
    MysqlAlterQuery& DropColumn(const std::string &field_name);
    MysqlAlterQuery& ModifyColumn(MysqlField& field);
    MysqlAlterQuery& ChangeColumn(const std::string& ofield_name, MysqlField& field);
};


class MysqlResult
{
public:
    using MysqlResultTable = std::vector<std::vector<std::string>>;
    MysqlResult(MYSQL* mysql, int result)
        : m_mysql(mysql), m_result(result) {}
    /*
    * select query result
    */
    MysqlResultTable ToResultTable();
    bool IsSuccess();
    std::string GetError();
    uint64_t GetAffectedRows();
    ~MysqlResult() = default;
private:
    MYSQL* m_mysql;
    int m_result;
};

class MysqlSession
{
public:
    using ptr = std::shared_ptr<MysqlSession>;
    using uptr = std::unique_ptr<MysqlSession>;

    MysqlSession(MysqlConfig::ptr config);
    MysqlSession(MysqlConfig::ptr config, Logger::ptr logger);
    bool Connect(const std::string& url);
    bool Connect(const std::string &host, const std::string &username, const std::string &password, const std::string &db_name, uint32_t port = 3306);
    void DisConnect();
    GHandle GetSocket();
    //如果断线重连
    bool PingAndReconnect();

    MysqlResult CreateTable(MysqlTable& table);
    MysqlResult DropTable(const std::string& table);
    MysqlResult Select(MysqlSelectQuery& query);
    MysqlResult Insert(MysqlInsertQuery& query);
    MysqlResult Update(MysqlUpdateQuery& query);
    MysqlResult Delete(MysqlDeleteQuery& query);
    MysqlResult Alter(MysqlAlterQuery& query);
    MysqlResult StartTransaction();
    MysqlResult Commit();
    MysqlResult Rollback();

    MysqlResult MysqlQuery(const std::string &query);

    MysqlStmtExecutor::ptr GetMysqlStmtExecutor();

    ~MysqlSession();
private:
    MYSQL *m_mysql; 
    Logger::ptr m_logger;
};

//To Do
class AsyncMysqlSession
{
public:
    using ptr = std::shared_ptr<AsyncMysqlSession>;
    using uptr = std::unique_ptr<AsyncMysqlSession>;
    AsyncMysqlSession(MysqlConfig::ptr config);
    Awaiter<bool> AsyncConnect(const std::string &host, const std::string &username, const std::string &password, const std::string &db_name, uint32_t port = 3306);

    ~AsyncMysqlSession();
private:
    MYSQL* m_mysql;
};

template<typename SessionType>
class MysqlClient {
public:
    MysqlClient(MysqlConfig::ptr config) {
        mysql_library_init(0, nullptr, nullptr);
        m_session = std::make_unique<SessionType>(config);
    }
    SessionType* GetSession() {
        return m_session.get();
    }
    ~MysqlClient(){
        mysql_library_end();
    }
private:
    std::unique_ptr<SessionType> m_session;
};

}

#include "Mysql.tcc"

#endif
