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
    MysqlStmtExecutor(MYSQL_STMT* stmt);
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
    std::string m_type;
    bool m_unique = false;
    bool m_autoincr = false;
    bool m_notnull = false;
    std::string m_default;
    std::string m_comment;
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

class MysqlSession
{
public:
    using ptr = std::shared_ptr<MysqlSession>;
    using uptr = std::unique_ptr<MysqlSession>;

    MysqlSession(MysqlConfig::ptr config);
    
    bool Connect(const std::string &host, const std::string &username, const std::string &password, const std::string &db_name, uint32_t port = 3306);
    void DisConnect();
    GHandle GetSocket();
    //如果断线重连
    bool PingAndReconnect();

    bool CreateTable(const MysqlTable& table);
    bool DropTable(const std::string& table);
    std::vector<std::vector<std::string>> Select(const std::string &TableName, const std::vector<std::string> &Fields, const std::string &Cond = "");
    bool Insert(const std::string &TableName, const std::vector<std::pair<std::string, std::string>> &Field_Value);
    bool Update(const std::string &TableName, const std::vector<std::pair<std::string, std::string>> &Field_Value, const std::string &Cond = "");
    bool Delete(const std::string &TableName, const std::string &Cond);
    bool AddField(const std::string &TableName, const std::pair<std::string, std::string> &Field_Type);
    bool ModFieldType(const std::string &TableName, const std::pair<std::string, std::string> &Field_Type);
    bool ModFieldName(const std::string &TableName, const std::string &OldFieldName, const std::pair<std::string, std::string> &Field_Type);
    bool DelField(const std::string &TableName, const std::string &FieldName);
    bool StartTransaction();
    bool Commit();
    bool Rollback();
    bool MysqlQuery(const std::string &query);

    MysqlStmtExecutor::ptr GetMysqlStmtExecutor();
    std::string GetLastError();

    ~MysqlSession();
private:
    MYSQL *m_mysql; 
};

//To Do
class AsyncMysqlSession
{
public:
    using ptr = std::shared_ptr<AsyncMysqlSession>;
    using uptr = std::unique_ptr<AsyncMysqlSession>;
    AsyncMysqlSession(MysqlConfig::ptr config);
    coroutine::Awaiter_bool AsyncConnect(const std::string &host, const std::string &username, const std::string &password, const std::string &db_name, uint32_t port = 3306);

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

#endif
