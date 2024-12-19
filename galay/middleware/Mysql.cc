#include "Mysql.h"
#include "galay/kernel/Log.h"

namespace galay::middleware::mysql
{
MySqlException::MySqlException(std::string message) 
    : m_message(message) 
{}

const char*
MySqlException::what() const noexcept 
{
    return m_message.c_str();
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

MysqlClient::MysqlClient(std::string charset)
{
    mysql_library_init(0,NULL,NULL);
    this->m_handle=mysql_init(NULL);
    if(this->m_handle==NULL) {
        LogError("[Error: {}]", mysql_error(this->m_handle));
        throw MySqlException(mysql_error(this->m_handle));
    }
    if(mysql_options(this->m_handle,MYSQL_SET_CHARSET_NAME,charset.c_str()) == -1){
        LogError("[Error: {}]", mysql_error(this->m_handle));
        throw MySqlException( mysql_error(this->m_handle));
    }
    uint32_t timeout = MYSQL_TIMEOUT;
    if(mysql_options(this->m_handle,MYSQL_OPT_CONNECT_TIMEOUT,&timeout) == -1){
        LogError("[Error: {}]", mysql_error(this->m_handle));
        throw MySqlException( mysql_error(this->m_handle));
    }
    if(mysql_options(this->m_handle,MYSQL_OPT_READ_TIMEOUT,&timeout) == -1){
        LogError("[Error: {}]", mysql_error(this->m_handle));
        throw MySqlException( mysql_error(this->m_handle));
    }
    if(mysql_options(this->m_handle,MYSQL_OPT_WRITE_TIMEOUT,&timeout) == -1){
        LogError("[Error: {}]", mysql_error(this->m_handle));
        throw MySqlException( mysql_error(this->m_handle));
    }
}

int 
MysqlClient::Connect(const std::string& Remote,const std::string& UserName,const std::string& Password,const std::string& DBName, uint32_t Port)
{
    if (!mysql_real_connect(this->m_handle, Remote.c_str(), UserName.c_str(), Password.c_str(), DBName.c_str(), Port, NULL, 0))
    {
        LogError("[Error: {}]", mysql_error(this->m_handle));
        return mysql_errno(this->m_handle);
    }
    return 0;
}

void 
MysqlClient::DisConnect()
{
    if(this->m_handle) 
    {
        mysql_close(this->m_handle);
        this->m_handle = nullptr;
    }
}

int 
MysqlClient::GetSocket()
{
    return m_handle->net.fd;
}

int 
MysqlClient::CreateTable(std::string TableName, const std::vector<std::tuple<std::string,std::string,std::string>>& Field_Type_Key)
{
    if(TableName.empty() || Field_Type_Key.empty())
    {
        LogError("[Arg: [TableName] or [Field_Type_Key] empty]");
        return -1;
    }    
    std::string query="CREATE TABLE IF NOT EXISTS " + TableName + "(";
    int n = Field_Type_Key.size();
    for(int i = 0 ; i < n ; ++i){
        auto [field,type,key] = Field_Type_Key[i];
        query = query + field + " " + type + " " + key ;
        if( i != n - 1) query += ",";
        else query += ");";
    }
    if (mysql_ping(this->m_handle))
    {
        LogError("[Error: {}]", mysql_error(this->m_handle));
        return -1;
    }
    LogInfo("[Query: {}]", query);
    int ret = mysql_real_query(this->m_handle,query.c_str(),query.size());
    if(ret){
        LogError("[Error: {}]", mysql_error(this->m_handle));
        return -1;
    }
    return 0;
}

int 
MysqlClient::DropTable(std::string TableName)
{
    if(TableName.empty()){
        LogError("[Arg: [TableName] empty]");
        return -1;
    }
    std::string query = "DROP TABLE IF EXISTS "+TableName+";";
    if (mysql_ping(this->m_handle))
    {
        LogError("[Error: {}]", mysql_error(this->m_handle));
        return -1;
    }
    LogInfo("[Query: {}]", query);
    int ret=mysql_real_query(this->m_handle,query.c_str(),query.size());
    if(ret){
        LogError("[Error: {}]", mysql_error(this->m_handle));
        return mysql_errno(this->m_handle);
    }
    return ret;
}

std::vector<std::vector<std::string>> 
MysqlClient::Select(const std::string& TableName,const std::vector<std::string> &Fields,const std::string& Cond)
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
    if (mysql_ping(this->m_handle))
    {
        LogError("[Error: {}]", mysql_error(this->m_handle));
        return {};
    }
    LogInfo("[Query: {}]", query);
    int ret = mysql_real_query(this->m_handle, query.c_str(),query.size());
    if (ret)
    {
        LogError("[Error: {}]", mysql_error(this->m_handle));
        return {};
    }
    MYSQL_RES *m_res = mysql_store_result(this->m_handle);
    if(m_res == nullptr) {
        LogError("[Error: {}]", mysql_error(this->m_handle));
        return {};
    }
    int field_num = mysql_num_fields(m_res); // 列数
    int row_num = mysql_num_rows(m_res);
    MYSQL_FIELD *fields; // 列名
    std::vector<std::vector<std::string>> result(row_num + 1,std::vector<std::string>(field_num));
    int col_start = 0;
    while ((fields = mysql_fetch_field(m_res)) != NULL)
    {
        result[0][col_start++] = std::string(fields->name);
    }
    MYSQL_ROW row;
    int row_start = 1;
    while ((row = mysql_fetch_row(m_res)) != NULL)
    {
        for (int i = 0; i < field_num; ++i)
        {
            if (row[i] == NULL)
            {
                result[row_start][i] = "NULL";
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

int 
MysqlClient::Insert(const std::string& TableName , const std::vector<std::pair<std::string,std::string>>& Field_Value)
{
    if (TableName.empty() || Field_Value.empty())
    {
        LogError("[Arg: [TableName] or [Field_Value] empty]");
        return -1;
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
    if (mysql_ping(this->m_handle))
    {
        LogError("[Error: {}]", mysql_error(this->m_handle));
        return -1;
    }
    LogInfo("[Query: {}]", query);
    int ret = mysql_real_query(this->m_handle, query.c_str(),query.size());
    if(ret){
        LogError("[Error: {}]", mysql_error(this->m_handle));
        return -1;
    }
    return 0;
}

int
MysqlClient::Update(const std::string& TableName,const std::vector<std::pair<std::string,std::string>>& Field_Value,const std::string& Cond)
{
    if(TableName.empty() || Field_Value.empty()){
        LogError("[Arg: [TableName] or [Field_Value] empty]");
        return -1;
    }
    std::string query = "UPDATE " + TableName + " SET ";
    int n = Field_Value.size();
    for (int i = 0; i < n; ++i)
    {
        query = query + Field_Value[i].first + "=" + Field_Value[i].second;
        if (i != n - 1) query += ",";
    }
    query = query + " WHERE " + Cond + ";";
    if (mysql_ping(this->m_handle))
    {
        LogError("[Error: {}]", mysql_error(this->m_handle));
        return -1;
    }
    LogInfo("[Query: {}]", query);
    int ret = mysql_real_query(this->m_handle, query.c_str(),query.size());
    if(ret){
        LogError("[Error: {}]", mysql_error(this->m_handle));
        return -1;
    }
    return 0;
}

int 
MysqlClient::Delete(const std::string& TableName, const std::string& Cond)
{
    if(TableName.empty() || Cond.empty())
    {
        LogError("[Arg: [TableName] or [Field_Value] empty]");
        return -1;
    }
    std::string query = "DELETE FROM " + TableName + " WHERE " + Cond + ";";
    if (mysql_ping(this->m_handle))
    {
        LogError("[Error: {}]", mysql_error(this->m_handle));
        return -1;
    }
    LogInfo("[Query: {}]", query);
    int ret = mysql_real_query(this->m_handle, query.c_str(),query.size());
    if(ret){
        LogError("[Error: {}]", mysql_error(this->m_handle));
        return -1;
    }
    return 0;
}


int 
MysqlClient::AddField(const std::string& TableName, const std::pair<std::string,std::string>& Field_Type)
{
    if (TableName.empty())
    {
        LogError("Arg: [TableName] empty");
        return -1;
    }
    std::string query = "ALTER TABLE " + TableName + " ADD COLUMN " + Field_Type.first + " " + Field_Type.second + ';';
    if (mysql_ping(this->m_handle))
    {
        LogError("[Error: {}]", mysql_error(this->m_handle));
        return -1;
    }
    LogInfo("[Query: {}]", query);
    int ret = mysql_real_query(this->m_handle, query.c_str(), query.size());
    if (ret)
    {
        LogError("[Error: {}]", mysql_error(this->m_handle));
        return -1;
    }
    return 0;
}

int 
MysqlClient::ModFieldType(const std::string& TableName, const std::pair<std::string,std::string>& Field_Type)
{
    if (TableName.empty())
    {
        LogError("[Arg: [TableName] empty]");
        return -1;
    }
    std::string query = "ALTER TABLE " + TableName + " MODIFY COLUMN " + Field_Type.first + " " + Field_Type.second + ';';
    if (mysql_ping(this->m_handle))
    {
        LogError("[Error: {}]", mysql_error(this->m_handle));
        return -1;
    }
    LogInfo("[Query: {}]", query);
    int ret = mysql_real_query(this->m_handle, query.c_str(), query.size());
    if (ret)
    {
        LogError("[Error: {}]", mysql_error(this->m_handle));
        return -1;
    }
    return 0;
}

int 
MysqlClient::ModFieldName(const std::string& TableName,const std::string& OldFieldName, const std::pair<std::string,std::string>& Field_Type)
{
    if(TableName.empty() || OldFieldName.empty() ){
        LogError("[Arg: [TableName] of [OldFieldName] empty]");
        return -1;
    }
    std::string query = "ALTER TABLE " + TableName + " CHANGE COLUMN " + OldFieldName + " " + Field_Type.first + " " + Field_Type.second + ';';
    if (mysql_ping(this->m_handle))
    {
        LogError("[Error: {}]", mysql_error(this->m_handle));
        return -1;
    }
    LogInfo("[Query: {}]", query);
    int ret = mysql_real_query(this->m_handle, query.c_str(), query.size());
    if (ret)
    {
        LogError("[Error: {}]", mysql_error(this->m_handle));
        return -1;
    }
    return 0;
}


int 
MysqlClient::DelField(const std::string& TableName,const std::string& FieldName)
{
    if(TableName.empty() || FieldName.empty()){
        LogError("[Arg: [TableName] of [OldFieldName] empty]");
        return -1;
    }
    std::string query = "ALTER TABLE " + TableName + "DROP COLUMN " + FieldName + ";";
    if (mysql_ping(this->m_handle))
    {
        LogError("[Error: {}]", mysql_error(this->m_handle));
        return -1;
    }
    LogInfo("[Query: {}]", query);
    int ret = mysql_real_query(this->m_handle, query.c_str(), query.size());
    if (ret)
    {
        LogError("[Error: {}]", mysql_error(this->m_handle));
        return -1;
    }
    return 0;
}

MysqlStmtExecutor::ptr 
MysqlClient::GetMysqlParams()
{
    if (mysql_ping(this->m_handle))
    {
        LogError("[Error: {}]", mysql_error(this->m_handle));
        return nullptr;
    }
    MYSQL_STMT *stmt = mysql_stmt_init(this->m_handle);
    if(stmt == nullptr) {
        LogError("[Error: {}]", mysql_error(this->m_handle));
        return nullptr;
    }
    MysqlStmtExecutor::ptr params = std::make_shared<MysqlStmtExecutor>(stmt);
    return params;
}

bool 
MysqlClient::StartTransaction()
{
    if (mysql_ping(this->m_handle))
    {
        return -1;
    }
    LogInfo("[Query: start transaction]");
    if(mysql_real_query(this->m_handle,"start transaction;",18))
    {
        LogError("[Error: {}]", mysql_error(this->m_handle));
        return false;
    }
    return true;
}

bool 
MysqlClient::Commit()
{
    if (mysql_ping(this->m_handle))
    {
        return -1;
    }
    LogInfo("[Query: commit]");
    if(mysql_real_query(this->m_handle,"commit;",7))
    {
        LogError("[Error: {}]", mysql_error(this->m_handle));
        return false;
    }
    return true;
}

bool 
MysqlClient::Rollback()
{
    if (mysql_ping(this->m_handle))
    {
        return -1;
    }
    LogInfo("[Query: rollback]");
    if(mysql_real_query(this->m_handle,"rollback;",9))
    {
        LogError("[Error: {}]", mysql_error(this->m_handle));
        return false;
    }
    return true;
}

std::string 
MysqlClient::GetLastError()
{
    return mysql_error(this->m_handle);
}

MysqlClient::~MysqlClient()
{
    if(this->m_handle)
    {
        mysql_close(this->m_handle);
        this->m_handle = nullptr;
    }
    mysql_library_end();
}

}