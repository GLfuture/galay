#include "Mysql.hpp"

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
    return *this;
}

MysqlField &MysqlField::Type(const std::string &type)
{
    m_type = type;
    return *this;
}

MysqlField &MysqlField::AutoIncrement()
{
    m_autoincr = true;
    return *this;
}

MysqlField &MysqlField::NotNull()
{
    m_notnull = true;
    return *this;
}

MysqlField &MysqlField::Unique()
{
    m_unique = true;
    return *this;
}

MysqlField &MysqlField::Default(const std::string &defaultValue)
{
    m_default = defaultValue;
    return *this;
}

MysqlField &MysqlField::Comment(const std::string &comment)
{
    m_comment = comment;
    return *this;
}


std::string MysqlField::ToString() const
{
    std::string result = m_name + " " + m_type;
    if (m_notnull) {
        result += " NOT NULL";
    }
    if (m_unique) {
        result += " UNIQUE";
    }
    if (!m_default.empty()) {
        result += " DEFAULT " + m_default;
    }
    if (m_autoincr) {
        result += " AUTO_INCREMENT";
    }
    if (!m_comment.empty()) {
        result += " COMMENT \"" + m_comment + "\"";
    }
    return std::move(result);
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
    std::string query = "CREATE TABLE IF NOT EXISTS ";
    std::string primaryinfo = "", foreigninfo = "";
    query = query + m_name + "(";
    for(int i = 0; i < m_primary_keys.size(); ++i) {
        query = query + m_primary_keys[i].ToString() + ",";
        if(i == 0){
            primaryinfo = "PRIMARY KEY(";
        } 
        primaryinfo = primaryinfo + m_primary_keys[i].m_name + ",";
        
        if(i == m_primary_keys.size() - 1) {
            primaryinfo[primaryinfo.length() - 1] = ')';
            primaryinfo += ',';
        } 
    }
    for(int i = 0; i < m_foreign_keys.size(); ++i) {
        query = query + m_foreign_keys[i].first.ToString() + ",";
        foreigninfo = foreigninfo + "FOREIGN KEY(" + m_foreign_keys[i].first.m_name + ") REFERENCES " \
            + m_foreign_keys[i].second + " ON DELETE CASCADE ON UPDATE CASCADE,";
    }
    for(auto& field :m_other_fields) {
        query = query + field.ToString() + ",";
    }
    query = query + primaryinfo + foreigninfo;
    query[query.length() - 1] = ')';
    return query + m_extra_action + ";";
}

MysqlStmtExecutor::MysqlStmtExecutor(MYSQL_STMT* stmt)
{
    this->m_stmt = stmt;
    this->m_storeResult = false;
}

bool 
MysqlStmtExecutor::Prepare(const std::string& ParamQuery)
{
    int ret = mysql_stmt_prepare(this->m_stmt, ParamQuery.c_str(), ParamQuery.length());
    if (ret)
    {
        LogError("[Error: {}]", this->m_stmt->last_error);
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
        LogError("[Error: {}]", this->m_stmt->last_error);
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
            LogError("[Error: {}]", this->m_stmt->last_error);
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
        LogError("[Error: {}]", this->m_stmt->last_error);
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
        LogError("[Error: {}]", this->m_stmt->last_error);
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
        LogError("[Error: {}]", this->m_stmt->last_error);
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
            LogError("[Error: {}]", this->m_stmt->last_error);
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
        LogError("[Error: {}]", this->m_stmt->last_error);
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
                LogError("[Error: {}]", this->m_stmt->last_error);
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
        LogError("[Error: {}]", this->m_stmt->last_error);
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
    this->m_mysql = mysql_init(nullptr);
    if(this->m_mysql==nullptr) {
        LogError("[Error: {}]", mysql_error(this->m_mysql));
        throw MySqlException(mysql_error(this->m_mysql));
    }
    if(mysql_options(this->m_mysql, MYSQL_SET_CHARSET_NAME, config->m_charset.c_str()) == -1){
        LogError("[Error: {}]", mysql_error(this->m_mysql));
        throw MySqlException( mysql_error(this->m_mysql));
    }
    if(config->m_connectTimeout != -1 && mysql_options(this->m_mysql,MYSQL_OPT_CONNECT_TIMEOUT,&(config->m_connectTimeout)) == -1){
        LogError("[Error: {}]", mysql_error(this->m_mysql));
        throw MySqlException( mysql_error(this->m_mysql));
    }
    if(config->m_readTimeout != -1 && mysql_options(this->m_mysql,MYSQL_OPT_READ_TIMEOUT,&(config->m_readTimeout)) == -1){
        LogError("[Error: {}]", mysql_error(this->m_mysql));
        throw MySqlException( mysql_error(this->m_mysql));
    }
    if(config->m_writeTimeout != -1 && mysql_options(this->m_mysql,MYSQL_OPT_WRITE_TIMEOUT,&(config->m_writeTimeout)) == -1){
        LogError("[Error: {}]", mysql_error(this->m_mysql));
        throw MySqlException( mysql_error(this->m_mysql));
    }
    if(config->m_compress && mysql_options(this->m_mysql, MYSQL_OPT_COMPRESS, nullptr)) {
        LogError("[Error: {}]", mysql_error(this->m_mysql));
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
            LogError("[Error: {}]", mysql_error(this->m_mysql));
            throw MySqlException( mysql_error(this->m_mysql));
        }
    }
}

bool 
MysqlSession::Connect(const std::string& host,const std::string& username,const std::string& password,const std::string& db_name, uint32_t port)
{
    if (!mysql_real_connect(this->m_mysql, host.c_str(), username.c_str(), password.c_str(), db_name.c_str(), port, nullptr, 0))
    {
        LogError("[Error: {}]", mysql_error(this->m_mysql));
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
                LogError("[Error: {}]", mysql_error(this->m_mysql));
                return false;
            }
        } else {
            LogError("[Error: {}]", mysql_error(this->m_mysql));
            return false;
        }
    }
    return true;
}

bool MysqlSession::CreateTable(const MysqlTable &table)
{
    std::string query = table.ToString();
    if(query.empty()){
        LogError("[Arg: table empty]");
        return false;
    }
    return MysqlQuery(query);
}

bool MysqlSession::DropTable(const std::string &table)
{
    if(table.empty()){
        LogError("[Arg: table name empty]");
        return false;
    }
    std::string query = "DROP TABLE IF EXISTS " + table + ";";
    return MysqlQuery(query);
}

std::vector<std::vector<std::string>> 
MysqlSession::Select(const std::string& TableName,const std::vector<std::string> &Fields,const std::string& Cond)
{
    if (Fields.empty() || TableName.empty())
    {
        LogError("[Arg: [Fields] or [TableName] empty]");
        return {};
    }
    std::string query = "SELECT " ;
    int n = Fields.size();
    for (int i = 0; i < n; i++)
    {
        query += Fields[i];
        if (i != n - 1) query += ",";
    }
    query = query + " FROM " + TableName;
    if (!Cond.empty())
    {
        query += " WHERE " + Cond;
    }
    query += ";";
    if(!MysqlQuery(query)) {
        return {};
    }
    MYSQL_RES *m_res = mysql_store_result(this->m_mysql);
    if(m_res == nullptr) {
        LogError("[Error: {}]", mysql_error(this->m_mysql));
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
            if (row[i] == nullptr)
            {
                result[row_start][i] = "nullptr";
            }
            else
            {
                result[row_start][i] = row[i];
            }
        }
        ++row_start;
    }
    mysql_free_result(m_res);
    return result;
}

bool 
MysqlSession::Insert(const std::string& TableName , const std::vector<std::pair<std::string,std::string>>& Field_Value)
{
    if (TableName.empty() || Field_Value.empty())
    {
        LogError("[Arg: [TableName] or [Field_Value] empty]");
        return false;
    }
    int n = Field_Value.size();
    std::string fields = "(", values = "(";
    for(int i = 0 ; i < n ; ++i ) {
        fields += Field_Value[i].first;
        values += Field_Value[i].second;
        if(i != n - 1) {
            fields += ',';
            values += ',';
        }else {
            fields += ')';
            values += ')';
        }
    }
    std::string query = "INSERT INTO " + TableName + fields + " values " + values + ";";
    return MysqlQuery(query);
}

bool
MysqlSession::Update(const std::string& TableName,const std::vector<std::pair<std::string,std::string>>& Field_Value,const std::string& Cond)
{
    if(TableName.empty() || Field_Value.empty()){
        LogError("[Arg: [TableName] or [Field_Value] empty]");
        return false;
    }
    std::string query = "UPDATE " + TableName + " SET ";
    int n = Field_Value.size();
    for (int i = 0; i < n; ++i)
    {
        query = query + Field_Value[i].first + "=" + Field_Value[i].second;
        if (i != n - 1) query += ",";
    }
    query = query + " WHERE " + Cond + ";";
    return MysqlQuery(query);
}

bool 
MysqlSession::Delete(const std::string& TableName, const std::string& Cond)
{
    if(TableName.empty() || Cond.empty())
    {
        LogError("[Arg: [TableName] or [Field_Value] empty]");
        return false;
    }
    std::string query = "DELETE FROM " + TableName + " WHERE " + Cond + ";";
    return MysqlQuery(query);
}


bool 
MysqlSession::AddField(const std::string& TableName, const std::pair<std::string,std::string>& Field_Type)
{
    if (TableName.empty())
    {
        LogError("Arg: [TableName] empty");
        return false;
    }
    std::string query = "ALTER TABLE " + TableName + " ADD COLUMN " + Field_Type.first + " " + Field_Type.second + ';';
    return MysqlQuery(query);
}

bool 
MysqlSession::ModFieldType(const std::string& TableName, const std::pair<std::string,std::string>& Field_Type)
{
    if (TableName.empty())
    {
        LogError("[Arg: [TableName] empty]");
        return false;
    }
    std::string query = "ALTER TABLE " + TableName + " MODIFY COLUMN " + Field_Type.first + " " + Field_Type.second + ';';
    return MysqlQuery(query);
}

bool 
MysqlSession::ModFieldName(const std::string& TableName,const std::string& OldFieldName, const std::pair<std::string,std::string>& Field_Type)
{
    if(TableName.empty() || OldFieldName.empty() ){
        LogError("[Arg: [TableName] of [OldFieldName] empty]");
        return false;
    }
    std::string query = "ALTER TABLE " + TableName + " CHANGE COLUMN " + OldFieldName + " " + Field_Type.first + " " + Field_Type.second + ';';
    return MysqlQuery(query);
}


bool 
MysqlSession::DelField(const std::string& TableName,const std::string& FieldName)
{
    if(TableName.empty() || FieldName.empty()){
        LogError("[Arg: [TableName] of [OldFieldName] empty]");
        return false;
    }
    std::string query = "ALTER TABLE " + TableName + "DROP COLUMN " + FieldName + ";";
    return MysqlQuery(query);
}

MysqlStmtExecutor::ptr 
MysqlSession::GetMysqlStmtExecutor()
{
    if (mysql_ping(this->m_mysql))
    {
        LogError("[Error: {}]", mysql_error(this->m_mysql));
        return nullptr;
    }
    MYSQL_STMT *stmt = mysql_stmt_init(this->m_mysql);
    if(stmt == nullptr) {
        LogError("[Error: {}]", mysql_error(this->m_mysql));
        return nullptr;
    }
    MysqlStmtExecutor::ptr params = std::make_shared<MysqlStmtExecutor>(stmt);
    return params;
}

bool 
MysqlSession::StartTransaction()
{
    return MysqlQuery("start transaction;");
}

bool 
MysqlSession::Commit()
{
    return MysqlQuery("commit;");
}

bool 
MysqlSession::Rollback()
{
    return MysqlQuery("rollback;");
}

bool MysqlSession::MysqlQuery(const std::string &query)
{
    LogInfo("[Query: {}]", query);
    int ret = mysql_real_query(this->m_mysql,query.c_str(),query.size());
    if(ret){
        LogError("[Error: {}]", mysql_error(this->m_mysql));
        return false;
    }
    return true;
}

std::string 
MysqlSession::GetLastError()
{
    return mysql_error(this->m_mysql);
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
    this->m_mysql = mysql_init(nullptr);
    if(this->m_mysql==nullptr) {
        LogError("[Error: {}]", mysql_error(this->m_mysql));
        throw MySqlException(mysql_error(this->m_mysql));
    }
    if(mysql_options(this->m_mysql, MYSQL_SET_CHARSET_NAME, config->m_charset.c_str()) == -1){
        LogError("[Error: {}]", mysql_error(this->m_mysql));
        throw MySqlException( mysql_error(this->m_mysql));
    }
    if(config->m_connectTimeout != -1 && mysql_options(this->m_mysql,MYSQL_OPT_CONNECT_TIMEOUT,&(config->m_connectTimeout)) == -1){
        LogError("[Error: {}]", mysql_error(this->m_mysql));
        throw MySqlException( mysql_error(this->m_mysql));
    }
    if(config->m_readTimeout != -1 && mysql_options(this->m_mysql,MYSQL_OPT_READ_TIMEOUT,&(config->m_readTimeout)) == -1){
        LogError("[Error: {}]", mysql_error(this->m_mysql));
        throw MySqlException( mysql_error(this->m_mysql));
    }
    if(config->m_writeTimeout != -1 && mysql_options(this->m_mysql,MYSQL_OPT_WRITE_TIMEOUT,&(config->m_writeTimeout)) == -1){
        LogError("[Error: {}]", mysql_error(this->m_mysql));
        throw MySqlException( mysql_error(this->m_mysql));
    }
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