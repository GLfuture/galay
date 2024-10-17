#pragma once
#ifndef GALAY_MYSQL_H
#define GALAY_MYSQL_H

#include <mysql/mysql.h>
#include <string>
#include <vector>
#include <typeinfo>
#include <cxxabi.h>
#include <memory>
#include <string.h>

namespace galay::middleware::mysql
{
    #define MYSQL_TIMEOUT 5
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

    class MysqlClient //: public std::exception
    {
    private:
        MysqlClient(const MysqlClient &) = delete;
        MysqlClient(MysqlClient &&) = delete;
        MysqlClient &operator=(const MysqlClient &) = delete;
        MysqlClient &operator=(const MysqlClient &&) = delete;
    private:
        MYSQL *m_handle; // mysql连接句柄结构体
    public:
        using ptr = std::shared_ptr<MysqlClient>;
        using uptr = std::unique_ptr<MysqlClient>;

        MysqlClient(std::string charset = "utf8mb4");
        int Connect(const std::string &Remote, const std::string &UserName, const std::string &Password, const std::string &DBName, uint16_t Port = 3306);
        void DisConnect();
        int GetSocket();
        int CreateTable(std::string TableName, const std::vector<std::tuple<std::string, std::string, std::string>> &Field_Type_Key);
        int DropTable(std::string TableName);
        std::vector<std::vector<std::string>> Select(const std::string &TableName, const std::vector<std::string> &Fields, const std::string &Cond = "");
        int Insert(const std::string &TableName, const std::vector<std::pair<std::string, std::string>> &Field_Value);
        int Update(const std::string &TableName, const std::vector<std::pair<std::string, std::string>> &Field_Value, const std::string &Cond = "");
        int Delete(const std::string &TableName, const std::string &Cond);
        int AddField(const std::string &TableName, const std::pair<std::string, std::string> &Field_Type);
        int ModFieldType(const std::string &TableName, const std::pair<std::string, std::string> &Field_Type);
        int ModFieldName(const std::string &TableName, const std::string &OldFieldName, const std::pair<std::string, std::string> &Field_Type);
        int DelField(const std::string &TableName, const std::string &FieldName);
        bool StartTransaction();
        bool Commit();
        bool Rollback();

        MysqlStmtExecutor::ptr GetMysqlParams();
        std::string GetLastError();

        ~MysqlClient();
    };
}

#endif
