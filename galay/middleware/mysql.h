#pragma once
#ifndef GALAY_MYSQL_H
#define GALAY_MYSQL_H

#include <mysql/mysql.h>
#include <string>
#include <vector>
#include <typeinfo>
#include <cxxabi.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <string.h>

using namespace abi;

namespace galay::middleware::mysql
{
    
    class MySqlException : public std::exception
    {
    public:
        MySqlException(std::string message);

        virtual const char *what() const noexcept override;

    private:
        std::string m_message;
    };

    class MysqlClient //: public std::exception
    {
    private:
        bool m_connected = false;
        MYSQL *m_handle; // mysql连接句柄结构体
    public:
        using ptr = std::shared_ptr<MysqlClient>;
        using uptr = std::unique_ptr<MysqlClient>;

        MysqlClient(const MysqlClient &) = delete;
        MysqlClient(MysqlClient &&) = delete;
        MysqlClient &operator=(const MysqlClient &) = delete;
        MysqlClient &operator=(const MysqlClient &&) = delete;

        MysqlClient(std::string charset = "utf8mb4");

        int Connect(const std::string &Remote, const std::string &UserName, const std::string &Password, const std::string &DBName, uint16_t Port = 3306);

        void DisConnect();

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

        // 发送二进制数据
        int ParamSendBinaryData(const std::string &ParamQuery, const std::string &Data);
        // 接收二进制数据
        int ParamRecvBinaryData(const std::string &ParamQuery, std::string &Data);
        // 开启事务
        bool StartTransaction();
        // 提交事务
        bool Commit();
        // 回滚事务
        bool Rollback();

        ~MysqlClient();
    };
}

#endif
