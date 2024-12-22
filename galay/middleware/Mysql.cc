#include "Mysql.hpp"
#include <regex>

namespace galay::mysql
{
MySqlException::MySqlException(std::string message) 
    : m_message(message) 
{}

const char*
MySqlException::what() const noexcept 
{
    return m_message.c_str();
}

MysqlConfig::ptr MysqlConfig::CreateConfig()
{
    return std::make_shared<MysqlConfig>();
}

MysqlConfig& MysqlConfig::WithCharset(const std::string &charset)
{
    m_charset = charset;
    return *this;
}

MysqlConfig& MysqlConfig::WithConnectTimeout(uint32_t timeout)
{
    m_connectTimeout = timeout;
    return *this;
}

MysqlConfig& MysqlConfig::WithReadTimeout(uint32_t timeout)
{
    m_readTimeout = timeout;
    return *this;
}

MysqlConfig& MysqlConfig::WithWriteTimeout(uint32_t timeout)
{
    m_writeTimeout = timeout;
    return *this;
}

MysqlConfig& MysqlConfig::WithCompress(bool enable)
{
    m_compress = enable;
    return *this;
}

MysqlConfig& MysqlConfig::WithSSL(MysqlSSLSettings setting)
{
    m_enableSSL = true;
    m_ssl_setting = setting;
    return *this;
}

MysqlField &MysqlField::Name(const std::string &name)
{
    m_name = name;
    m_stream << name;
    return *this;
}

MysqlField &MysqlField::Type(const std::string &type)
{
    m_stream << " " << type;
    return *this;
}

MysqlField &MysqlField::NotNull()
{
    m_stream << " NOT NULL";
    return *this;
}

MysqlField &MysqlField::AutoIncrement()
{
    m_stream << " AUTO_INCREMENT";
    return *this;
}


MysqlField &MysqlField::Unique()
{
    m_stream << " UNIQUE";
    return *this;
}

MysqlField &MysqlField::Default(const std::string &defaultValue)
{
    m_stream << " DEFAULT " << defaultValue;
    return *this;
}

MysqlField &MysqlField::Comment(const std::string &comment)
{
    m_stream << " COMMENT \"" << comment << "\"";
    return *this;
}


std::string MysqlField::ToString() const
{
    return m_stream.str();
}

MysqlTable::MysqlTable(const std::string &name)
    :m_name(name)
{
}

MysqlField &MysqlTable::PrimaryKey()
{
    m_primary_keys.emplace_back();
    return m_primary_keys.back();
}

MysqlField &MysqlTable::ForeignKey(const std::string &name)
{
    m_foreign_keys.emplace_back(MysqlField{}, name);
    return m_foreign_keys.back().first;
}

MysqlField &MysqlTable::CreateUpdateTimeStampField()
{
    m_other_fields.emplace_back();
    m_other_fields.back().Type("TIMESTAMP").Default("CURRENT_TIMESTAMP");
    return m_other_fields.back();
}

MysqlTable &MysqlTable::ExtraAction(const std::string &action)
{
    m_extra_action += action;
    return *this;
}

std::string MysqlTable::ToString() const
{
    std::ostringstream query, primaryinfo, foreigninfo;
    query << "CREATE TABLE IF NOT EXISTS ";
    query << m_name << "(";
    for(int i = 0; i < m_primary_keys.size(); ++i) {
        query << m_primary_keys[i].ToString() << ",";
        if(i == 0){
            primaryinfo << "PRIMARY KEY(";
        } 
        if(i == m_primary_keys.size() - 1) {
            primaryinfo << ')';
        }
        primaryinfo << m_primary_keys[i].m_name << ","; 
    }
    for(int i = 0; i < m_foreign_keys.size(); ++i) {
        query << m_foreign_keys[i].first.ToString() << ",";
        foreigninfo << "FOREIGN KEY(" << m_foreign_keys[i].first.m_name << ") REFERENCES " \
            << m_foreign_keys[i].second << " ON DELETE CASCADE ON UPDATE CASCADE,";
    }
    for(auto& field :m_other_fields) {
        query << field.ToString() + ",";
    }
    std::string temp = foreigninfo.str();
    temp[temp.length() - 1] = ')';
    query << primaryinfo.str() << temp << m_extra_action << ";";
    return query.str();
}

std::string OrderByToString(MysqlOrderBy orderby)
{
    switch (orderby)
    {
    case MysqlOrderBy::ASC:
        return "ASC";
    case MysqlOrderBy::DESC:
        return "DESC";
    default:
        break;
    }
    return "";
}

std::string MysqlQuery::ToString()
{
    m_stream << ";";
    std::string query = m_stream.str();
    m_stream.str("");
    return query;
}

MysqlSelectQuery &MysqlSelectQuery::InnerJoinOn(const std::string &table, const std::string &condition)
{
    m_stream << " INNER JOIN " << table << " ON " << condition;
    return *this;
}

MysqlSelectQuery &MysqlSelectQuery::LeftJoinOn(const std::string &table, const std::string &condition)
{
    m_stream << " LEFT JOIN " << table << " ON " << condition;
    return *this;
}

MysqlSelectQuery &MysqlSelectQuery::RightJoinOn(const std::string &table, const std::string &condition)
{
    m_stream << " RIGHT JOIN " << table << " ON " << condition;
    return *this;
}

MysqlSelectQuery &MysqlSelectQuery::FullJoinOn(const std::string &table, const std::string &condition)
{
    m_stream << " FULL JOIN " << table << " ON " << condition;
    return *this;
}

MysqlSelectQuery &MysqlSelectQuery::CrossJoin(const std::string &table)
{
    m_stream << " CROSS JOIN " << table;
    return *this;
}

MysqlSelectQuery &MysqlSelectQuery::Union()
{
    m_stream << " UNION ";
    return *this;
}

MysqlSelectQuery &MysqlSelectQuery::Where(const std::string &condition)
{
    m_stream << " WHERE " << condition;
    return *this;
}


MysqlSelectQuery &MysqlSelectQuery::Having(const std::string &condition)
{
    m_stream << " HAVING " << condition;
    return *this;
}

MysqlSelectQuery &MysqlSelectQuery::Limit(uint32_t limit)
{
    m_stream << " LIMIT " << limit;
    return *this;
}

MysqlInsertQuery &MysqlInsertQuery::Table(const std::string &table)
{
    m_stream << "INSERT INTO " << table;
    return *this;
}


MysqlUpdateQuery &MysqlUpdateQuery::Table(const std::string &table)
{
    m_stream << "UPDATE " << table;
    return *this;
}

MysqlUpdateQuery &MysqlUpdateQuery::Where(const std::string &condition)
{
    m_stream << " WHERE " << condition;
    return *this;
}

MysqlDeleteQuery &MysqlDeleteQuery::Table(const std::string &table)
{
    m_stream << "DELETE FROM " << table;
    return *this;
}

MysqlDeleteQuery &MysqlDeleteQuery::Where(const std::string &condition)
{
    m_stream << " WHERE " << condition;
    return *this;
}

MysqlAlterQuery &MysqlAlterQuery::Table(const std::string &table)
{
    m_stream << "ALTER TABLE " << table;
    return *this;
}

MysqlAlterQuery &MysqlAlterQuery::AddColumn(MysqlField& field)
{
    m_stream << " ADD COLUMN " << field.ToString();
    return *this;
}

MysqlAlterQuery &MysqlAlterQuery::DropColumn(const std::string &field_name)
{
    m_stream << " DROP COLUMN " << field_name;
    return *this;
}

MysqlAlterQuery &MysqlAlterQuery::ModifyColumn(MysqlField& field)
{
    m_stream << " MODIFY COLUMN " << field.ToString();
    return *this;
}

MysqlAlterQuery &MysqlAlterQuery::ChangeColumn(const std::string &ofield_name, MysqlField& field)
{
    m_stream << " CHANGE COLUMN " << ofield_name << " " << field.ToString();
    return *this;
}

MysqlResult::MysqlResultTable MysqlResult::ToResultTable()
{
    MYSQL_RES *m_res = mysql_store_result(m_mysql);
    if(m_res == nullptr) {
        return {};
    }
    int field_num = mysql_num_fields(m_res); // 列数
    int row_num = mysql_num_rows(m_res);
    MYSQL_FIELD *fields; // 列名
    std::vector<std::vector<std::string>> result(row_num + 1,std::vector<std::string>(field_num));
    int col_start = 0;
    while ((fields = mysql_fetch_field(m_res)) != nullptr)
    {
        result[0][col_start++] = std::string(fields->name);
    }
    MYSQL_ROW row;
    int row_start = 1;
    while ((row = mysql_fetch_row(m_res)) != nullptr)
    {
        for (int i = 0; i < field_num; ++i)
        {
            if (row[i] == nullptr) {
                result[row_start][i] = "NULL";
            } else {
                result[row_start][i] = row[i];
            }
        }
        ++row_start;
    }
    mysql_free_result(m_res);
    return result;
}

bool MysqlResult::IsSuccess()
{
    return m_result == 0;
}

std::string MysqlResult::GetError()
{
    return mysql_error(m_mysql);
}

uint64_t MysqlResult::GetAffectedRows()
{
    return mysql_affected_rows(m_mysql);
}

MysqlStmtExecutor::MysqlStmtExecutor(MYSQL_STMT* stmt, Logger::ptr logger)
    : m_stmt(stmt), m_storeResult(false), m_logger(logger)
{
}

bool 
MysqlStmtExecutor::Prepare(const std::string& ParamQuery)
{
    int ret = mysql_stmt_prepare(this->m_stmt, ParamQuery.c_str(), ParamQuery.length());
    if (ret)
    {
        MysqlLogError(m_logger->SpdLogger(), "[Error: {}]", this->m_stmt->last_error);
        return false;
    }
    return true;
}

bool  
MysqlStmtExecutor::BindParam(MYSQL_BIND* params)
{
    int ret = mysql_stmt_bind_param(this->m_stmt, params);
    if (ret)
    {
        MysqlLogError(m_logger->SpdLogger(), "[Error: {}]", this->m_stmt->last_error);
        return false;
    }
    return 0;
}


bool  
MysqlStmtExecutor::StringToParam(const std::string& data, unsigned int index)
{
    unsigned long offset = 0;
    const unsigned long chunkSize = 1024;
    while (offset < data.length()) {
        unsigned long remaining = data.size() - offset;
        unsigned long bytesToSend = remaining < chunkSize? remaining : chunkSize;
        if (mysql_stmt_send_long_data(m_stmt, index, data.c_str() + offset, bytesToSend)!= 0) {
            MysqlLogError(m_logger->SpdLogger(), "[Error: {}]", this->m_stmt->last_error);
            return false;
        }
        offset += bytesToSend;
    }
    return true;
}

bool 
MysqlStmtExecutor::BindResult(MYSQL_BIND* result)
{
    int ret = mysql_stmt_bind_result(this->m_stmt, result);
    if (ret)
    {
        MysqlLogError(m_logger->SpdLogger(), "[Error: {}]", this->m_stmt->last_error);
        return false;
    }
    return true;
}

bool 
MysqlStmtExecutor::Execute()
{
    int ret = mysql_stmt_execute(this->m_stmt);
    if (ret)
    {
        MysqlLogError(m_logger->SpdLogger(), "[Error: {}]", this->m_stmt->last_error);
        return false;
    }
    return true;
}

bool 
MysqlStmtExecutor::StoreResult()
{
    int ret = mysql_stmt_store_result(this->m_stmt);
    if (ret)
    {
        MysqlLogError(m_logger->SpdLogger(), "[Error: {}]", this->m_stmt->last_error);
        return false;
    }
    return true;
}

bool 
MysqlStmtExecutor::GetARow(MYSQL_BIND* result)
{
    if(!this->m_storeResult)
    {
        if(!StoreResult())
        {
            MysqlLogError(m_logger->SpdLogger(), "[Error: {}]", this->m_stmt->last_error);
            return false;
        }
        m_storeResult = true;
    }
    int ret = mysql_stmt_fetch(this->m_stmt);
    if (ret == MYSQL_DATA_TRUNCATED || ret == MYSQL_NO_DATA)
    {
        m_storeResult = false;
        return false;
    }
    if(ret != 0)
    {
        m_storeResult = false;
        MysqlLogError(m_logger->SpdLogger(), "[Error: {}]", this->m_stmt->last_error);
        return false;
    }
    for(int i = 0 ; i < mysql_stmt_param_count(this->m_stmt); ++i)
    {
        if(result[i].buffer_type == MYSQL_TYPE_BLOB || result[i].buffer_type == MYSQL_TYPE_STRING
            || result[i].buffer_type == MYSQL_TYPE_VAR_STRING || result[i].buffer_type == MYSQL_TYPE_TINY_BLOB || result[i].buffer_type == MYSQL_TYPE_MEDIUM_BLOB || result[i].buffer_type == MYSQL_TYPE_LONG_BLOB)
        {
            char* str = new char[*result[i].length];
            bzero(str,*result[i].length);
            result[i].buffer = str;
            result[i].buffer_length = *result[i].length;
            if(mysql_stmt_fetch_column(this->m_stmt, &result[i], i, 0))
            {
                MysqlLogError(m_logger->SpdLogger(), "[Error: {}]", this->m_stmt->last_error);
                return false;
            }
        }
        else
        {
            mysql_stmt_fetch_column(this->m_stmt, &result[i], i, 0);
        }

    }
    return true;
}

bool 
MysqlStmtExecutor::Close()
{
    int ret = mysql_stmt_close(this->m_stmt);
    this->m_stmt = nullptr;
    if (ret)
    {
        MysqlLogError(m_logger->SpdLogger(), "[Error: {}]", this->m_stmt->last_error);
        return false;
    }
    return true;
}

std::string 
MysqlStmtExecutor::GetLastError()
{
    return mysql_stmt_error(this->m_stmt);
}

MysqlStmtExecutor::~MysqlStmtExecutor()
{

}

MysqlSession::MysqlSession(MysqlConfig::ptr config)
{
    auto logger = Logger::CreateStdoutLoggerMT("MysqlLogger");
    MysqlSession(config, logger);
}

MysqlSession::MysqlSession(MysqlConfig::ptr config, Logger::ptr logger)
    : m_logger(logger)
{
    this->m_mysql = mysql_init(nullptr);
    if(this->m_mysql==nullptr) {
        MysqlLogError(m_logger->SpdLogger(), "[Error: {}]", mysql_error(this->m_mysql));
        throw MySqlException(mysql_error(this->m_mysql));
    }
    if(mysql_options(this->m_mysql, MYSQL_SET_CHARSET_NAME, config->m_charset.c_str()) == -1){
        MysqlLogError(m_logger->SpdLogger(), "[Error: {}]", mysql_error(this->m_mysql));
        throw MySqlException( mysql_error(this->m_mysql));
    }
    if(config->m_connectTimeout != -1 && mysql_options(this->m_mysql,MYSQL_OPT_CONNECT_TIMEOUT,&(config->m_connectTimeout)) == -1){
        MysqlLogError(m_logger->SpdLogger(), "[Error: {}]", mysql_error(this->m_mysql));
        throw MySqlException( mysql_error(this->m_mysql));
    }
    if(config->m_readTimeout != -1 && mysql_options(this->m_mysql,MYSQL_OPT_READ_TIMEOUT,&(config->m_readTimeout)) == -1){
        MysqlLogError(m_logger->SpdLogger(), "[Error: {}]", mysql_error(this->m_mysql));
        throw MySqlException( mysql_error(this->m_mysql));
    }
    if(config->m_writeTimeout != -1 && mysql_options(this->m_mysql,MYSQL_OPT_WRITE_TIMEOUT,&(config->m_writeTimeout)) == -1){
        MysqlLogError(m_logger->SpdLogger(), "[Error: {}]", mysql_error(this->m_mysql));
        throw MySqlException( mysql_error(this->m_mysql));
    }
    if(config->m_compress && mysql_options(this->m_mysql, MYSQL_OPT_COMPRESS, nullptr)) {
        MysqlLogError(m_logger->SpdLogger(), "[Error: {}]", mysql_error(this->m_mysql));
        throw MySqlException( mysql_error(this->m_mysql));
    }
    if(config->m_enableSSL) {
        const char* capath = config->m_ssl_setting.capath.empty() ? nullptr : config->m_ssl_setting.capath.c_str();
        const char* cipher = config->m_ssl_setting.cipher.empty() ? nullptr : config->m_ssl_setting.cipher.c_str();
        int ret = mysql_options(this->m_mysql, MYSQL_OPT_SSL_KEY, config->m_ssl_setting.key.c_str());
        ret |= mysql_options(this->m_mysql, MYSQL_OPT_SSL_CERT, config->m_ssl_setting.cert.c_str());
        ret |= mysql_options(this->m_mysql, MYSQL_OPT_SSL_CA, config->m_ssl_setting.ca.c_str());
        ret |= mysql_options(this->m_mysql, MYSQL_OPT_SSL_CAPATH, capath);
        ret |= mysql_options(this->m_mysql, MYSQL_OPT_SSL_CIPHER, cipher);
        if(ret) {
            MysqlLogError(m_logger->SpdLogger(), "[Error: {}]", mysql_error(this->m_mysql));
            throw MySqlException( mysql_error(this->m_mysql));
        }
    }
}

bool MysqlSession::Connect(const std::string &url)
{
    std::regex re(R"(mysql://([^:@]+)(?::([^@]+))?@([^:/?]+)(?::(\d+))?(?:/([^?]*))?)");
    std::smatch matches;
    std::string username, password, host, database;
    uint32_t port = 3306;
    if (std::regex_search(url, matches, re)) {
        if (matches.size() > 1 && matches[1].matched) {
            username = matches[1].str();
        }
        if (matches.size() > 2 && matches[2].matched) {
            password = matches[2].str();
        }
        if (matches.size() > 3 && matches[3].matched) {
            host = matches[3].str();
        }
        if (matches.size() > 4 && matches[4].matched) {
            try
            {
                port = std::stoi(matches[4].str());
            }
            catch(const std::exception& e)
            {
                MysqlLogError(m_logger->SpdLogger(), "mysql url port error");
                return false;
            }
        }
        if (matches[5].matched) {
            database = matches[5].str();
        }
    } else {
        MysqlLogError(m_logger->SpdLogger(), "URL format is incorrect");
        return false;
    }
    return Connect(host, username, password, database, port);
}

bool 
MysqlSession::Connect(const std::string& host, const std::string& username, const std::string& password, const std::string& db_name, uint32_t port)
{
    if (!mysql_real_connect(this->m_mysql, host.c_str(), username.c_str(), password.c_str(), db_name.c_str(), port, nullptr, 0))
    {
        MysqlLogError(m_logger->SpdLogger(), "[Error: {}]", mysql_error(this->m_mysql));
        return false;
    }
    return true;
}

void 
MysqlSession::DisConnect()
{
    if(this->m_mysql) 
    {
        mysql_close(this->m_mysql);
        this->m_mysql = nullptr;
    }
}

GHandle 
MysqlSession::GetSocket()
{
    return {m_mysql->net.fd};
}

bool MysqlSession::PingAndReconnect()
{
    if (mysql_ping(this->m_mysql))
    {
        if(mysql_errno(this->m_mysql) == CR_SERVER_GONE_ERROR || mysql_errno(this->m_mysql) == CR_SERVER_LOST) {
            if(!Connect(this->m_mysql->host,this->m_mysql->user,this->m_mysql->passwd,this->m_mysql->db,this->m_mysql->port)){
                MysqlLogError(m_logger->SpdLogger(), "[Error: {}]", mysql_error(this->m_mysql));
                return false;
            }
        } else {
            MysqlLogError(m_logger->SpdLogger(), "[Error: {}]", mysql_error(this->m_mysql));
            return false;
        }
    }
    return true;
}

MysqlResult MysqlSession::CreateTable(MysqlTable &table)
{
    return MysqlQuery(table.ToString());
}

MysqlResult MysqlSession::DropTable(const std::string &table)
{
    std::string query = "DROP TABLE IF EXISTS " + table + ";";
    return MysqlQuery(query);
}

MysqlResult MysqlSession::Select(MysqlSelectQuery &query)
{
    return MysqlResult(MysqlQuery(query.ToString()));
}

MysqlResult 
MysqlSession::Insert(MysqlInsertQuery &query)
{
    return MysqlQuery(query.ToString());
}

MysqlResult
MysqlSession::Update(MysqlUpdateQuery& query)
{
    return MysqlQuery(query.ToString());
}

MysqlResult 
MysqlSession::Delete(MysqlDeleteQuery& query)
{
    return MysqlQuery(query.ToString());
}

MysqlResult MysqlSession::Alter(MysqlAlterQuery &query)
{
    return MysqlQuery(query.ToString());
}

MysqlStmtExecutor::ptr 
MysqlSession::GetMysqlStmtExecutor()
{
    if (mysql_ping(this->m_mysql))
    {
        MysqlLogError(m_logger->SpdLogger(), "[Error: {}]", mysql_error(this->m_mysql));
        return nullptr;
    }
    MYSQL_STMT *stmt = mysql_stmt_init(this->m_mysql);
    if(stmt == nullptr) {
        MysqlLogError(m_logger->SpdLogger(), "[Error: {}]", mysql_error(this->m_mysql));
        return nullptr;
    }
    MysqlStmtExecutor::ptr params = std::make_shared<MysqlStmtExecutor>(stmt, m_logger);
    return params;
}

MysqlResult 
MysqlSession::StartTransaction()
{
    return MysqlQuery("start transaction;");
}

MysqlResult 
MysqlSession::Commit()
{
    return MysqlQuery("commit;");
}

MysqlResult 
MysqlSession::Rollback()
{
    return MysqlQuery("rollback;");
}

MysqlResult MysqlSession::MysqlQuery(const std::string &query)
{
    MysqlLogInfo(m_logger->SpdLogger(), "[Mysql query: {}]", query);
    int ret = mysql_real_query(m_mysql, query.c_str(), query.size());
    return MysqlResult{m_mysql, ret};
}

MysqlSession::~MysqlSession()
{
    if(this->m_mysql)
    {
        mysql_close(this->m_mysql);
        this->m_mysql = nullptr;
    }
}

AsyncMysqlSession::AsyncMysqlSession(MysqlConfig::ptr config)
{
}


coroutine::Awaiter_bool 
AsyncMysqlSession::AsyncConnect(const std::string &host, const std::string &username, const std::string &password, const std::string &db_name, uint32_t port)
{
    net_async_status status = mysql_real_connect_nonblocking(this->m_mysql, host.c_str(), username.c_str(), password.c_str(), db_name.c_str(), port, nullptr, 0);
    if (status )
    {
    }

    return {true};
}

AsyncMysqlSession::~AsyncMysqlSession()
{
    if(this->m_mysql)
    {
        mysql_close(this->m_mysql);
        this->m_mysql = nullptr;
    }
}



}