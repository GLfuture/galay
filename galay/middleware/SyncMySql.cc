#include "SyncMySql.h"

galay::MiddleWare::MySql::MySqlException::MySqlException(std::string message) 
    : m_message(message) 
{}

const char*
galay::MiddleWare::MySql::MySqlException::what() const noexcept 
{
    return m_message.c_str();
}

galay::MiddleWare::MySql::SyncMySql::SyncMySql(std::string charset)
{
    this->m_handle=mysql_init(NULL);
    if(this->m_handle==NULL) {
        spdlog::error("[{}:{}] [mysql_init error: '{}']",__FILE__, __LINE__, mysql_error(this->m_handle));
        throw MySqlException(mysql_error(this->m_handle));
    }
    if(mysql_options(this->m_handle,MYSQL_SET_CHARSET_NAME,charset.c_str()) == -1){
        spdlog::error("[{}:{}] [mysql_options error: '{}']", __FILE__, __LINE__ , mysql_error(this->m_handle));
        throw MySqlException( mysql_error(this->m_handle));
    }
}

int 
galay::MiddleWare::MySql::SyncMySql::Connect(const std::string& Remote,const std::string& UserName,const std::string& Password,const std::string& DBName, uint16_t Port)
{
    if (!mysql_real_connect(this->m_handle, Remote.c_str(), UserName.c_str(), Password.c_str(), DBName.c_str(), Port, NULL, 0))
    {
        spdlog::error("[{}:{}] [mysql_connect error: '{}']",__FILE__,__LINE__,mysql_error(this->m_handle));
        return mysql_errno(this->m_handle);
    }
    this->m_connected = true;
    return 0;
}

void 
galay::MiddleWare::MySql::SyncMySql::DisConnect()
{
    if(this->m_connected) {
        mysql_close(this->m_handle);
        this->m_connected = false;
    }
}

int 
galay::MiddleWare::MySql::SyncMySql::CreateTable(std::string TableName, const std::vector<std::tuple<std::string,std::string,std::string>>& Field_Type_Key)
{
    if(TableName.empty() || Field_Type_Key.empty())
    {
        spdlog::error("[{}:{}] [Arg: [TableName] or [Field_Type_Key] empty]", __FILE__,__LINE__);
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
        spdlog::error("[{}:{}] [mysql_ping error: '{}']", __FILE__,__LINE__,mysql_error(this->m_handle));
        return -1;
    }
    spdlog::info("[{}:{}] [mysql query is '{}']", __FILE__,__LINE__,query);
    int ret = mysql_real_query(this->m_handle,query.c_str(),query.size());
    if(ret){
        spdlog::error("[{}:{}] [msyql_real_query error: '{}']", __FILE__,__LINE__,mysql_error(this->m_handle));
        return -1;
    }
    return 0;
}

int 
galay::MiddleWare::MySql::SyncMySql::DropTable(std::string TableName)
{
    if(TableName.empty()){
        spdlog::error("[{}:{}] [Arg: [TableName] empty]", __FILE__,__LINE__);
        return -1;
    }
    std::string query = "DROP TABLE IF EXISTS "+TableName+";";
    if (mysql_ping(this->m_handle))
    {
        spdlog::error("[{}:{}] [mysql_ping error: '{}']", __FILE__,__LINE__,mysql_error(this->m_handle));
        return -1;
    }
    spdlog::info("[{}:{}] [mysql query is '{}']", __FILE__,__LINE__,query);
    int ret=mysql_real_query(this->m_handle,query.c_str(),query.size());
    if(ret){
        spdlog::error("[{}:{}] [msyql_real_query error: '{}']", __FILE__,__LINE__,mysql_error(this->m_handle));
        return mysql_errno(this->m_handle);
    }
    return ret;
}

std::vector<std::vector<std::string>> 
galay::MiddleWare::MySql::SyncMySql::Select(const std::string& TableName,const std::vector<std::string> &Fields,const std::string& Cond)
{
    if (Fields.empty() || TableName.empty())
    {
        spdlog::error("[{}:{}] [Arg: [Fields] or [TableName] empty]", __FILE__,__LINE__);
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
        spdlog::error("[{}:{}] [mysql_ping error is '{}']", __FILE__,__LINE__,mysql_error(this->m_handle));
        return {};
    }
    spdlog::info("[{}:{}] [mysql query is '{}']", __FILE__,__LINE__,query);
    int ret = mysql_real_query(this->m_handle, query.c_str(),query.size());
    if (ret)
    {
        spdlog::error("[{}:{}] [msyql_real_query error: '{}']", __FILE__,__LINE__,mysql_error(this->m_handle));
        return {};
    }
    MYSQL_RES *m_res = mysql_store_result(this->m_handle);
    if(m_res == nullptr) {
        spdlog::error("[{}:{}] [mysql_store_result error: '{}']", __FILE__,__LINE__,mysql_error(this->m_handle));
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
galay::MiddleWare::MySql::SyncMySql::Insert(const std::string& TableName , const std::vector<std::pair<std::string,std::string>>& Field_Value)
{
    if (TableName.empty() || Field_Value.empty())
    {
        spdlog::error("[{}:{}] [Arg: [TableName] or [Field_Value] empty]",__FILE__,__LINE__);
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
        spdlog::error("[{}:{}] [mysql_ping error: '{}']", __FILE__,__LINE__,mysql_error(this->m_handle));
        return -1;
    }
    spdlog::info("[{}:{}] [mysql query is '{}']", __FILE__,__LINE__,query);
    int ret = mysql_real_query(this->m_handle, query.c_str(),query.size());
    if(ret){
        spdlog::error("[{}:{}] [msyql_real_query error: '{}']", __FILE__,__LINE__,mysql_error(this->m_handle));
        return -1;
    }
    return 0;
}

int
galay::MiddleWare::MySql::SyncMySql::Update(const std::string& TableName,const std::vector<std::pair<std::string,std::string>>& Field_Value,const std::string& Cond)
{
    if(TableName.empty() || Field_Value.empty()){
        spdlog::error("[{}:{}] [Arg: [TableName] or [Field_Value] empty]",__FILE__,__LINE__);
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
        spdlog::error("[{}:{}] [mysql_ping error: '{}']", __FILE__,__LINE__,mysql_error(this->m_handle));
        return -1;
    }
    spdlog::info("[{}:{}] [mysql query is '{}']", __FILE__,__LINE__,query);
    int ret = mysql_real_query(this->m_handle, query.c_str(),query.size());
    if(ret){
        spdlog::error("[{}:{}] [msyql_real_query error: '{}']", __FILE__,__LINE__,mysql_error(this->m_handle));
        return -1;
    }
    return 0;
}

int 
galay::MiddleWare::MySql::SyncMySql::Delete(const std::string& TableName, const std::string& Cond)
{
    if(TableName.empty() || Cond.empty())
    {
        spdlog::error("[{}:{}] [Arg: [TableName] or [Field_Value] empty]",__FILE__,__LINE__);
        return -1;
    }
    std::string query = "DELETE FROM " + TableName + " WHERE " + Cond + ";";
    if (mysql_ping(this->m_handle))
    {
        spdlog::error("[{}:{}] [mysql_ping error: '{}']", __FILE__,__LINE__,mysql_error(this->m_handle));
        return -1;
    }
    spdlog::info("[{}:{}] [mysql query is '{}']", __FILE__,__LINE__,query);
    int ret = mysql_real_query(this->m_handle, query.c_str(),query.size());
    if(ret){
        spdlog::error("[{}:{}] [msyql_real_query error: '{}']", __FILE__,__LINE__,mysql_error(this->m_handle));
        return -1;
    }
    return 0;
}


int 
galay::MiddleWare::MySql::SyncMySql::AddField(const std::string& TableName, const std::pair<std::string,std::string>& Field_Type)
{
    if (TableName.empty())
    {
        spdlog::error("[{}:{}] Arg: [TableName] empty", __TIME__, __FILE__, __LINE__);
        return -1;
    }
    std::string query = "ALTER TABLE " + TableName + " ADD COLUMN " + Field_Type.first + " " + Field_Type.second + ';';
    if (mysql_ping(this->m_handle))
    {
        spdlog::error("[{}:{}] [mysql_ping error: '{}']", __TIME__, __FILE__, __LINE__, mysql_error(this->m_handle));
        return -1;
    }
    spdlog::info("[{}:{}] [mysql query is '{}']", __TIME__, __FILE__, __LINE__, query);
    int ret = mysql_real_query(this->m_handle, query.c_str(), query.size());
    if (ret)
    {
        spdlog::error("[{}:{}] [msyql_real_query error: '{}']", __TIME__, __FILE__, __LINE__, mysql_error(this->m_handle));
        return -1;
    }
    return 0;
}

int 
galay::MiddleWare::MySql::SyncMySql::ModFieldType(const std::string& TableName, const std::pair<std::string,std::string>& Field_Type)
{
    if (TableName.empty())
    {
        spdlog::error("[{}:{}] [Arg: [TableName] empty]", __TIME__, __FILE__, __LINE__);
        return -1;
    }
    std::string query = "ALTER TABLE " + TableName + " MODIFY COLUMN " + Field_Type.first + " " + Field_Type.second + ';';
    if (mysql_ping(this->m_handle))
    {
        spdlog::error("[{}:{}] [mysql_ping error: '{}']", __TIME__, __FILE__, __LINE__, mysql_error(this->m_handle));
        return -1;
    }
    spdlog::info("[{}:{}] [mysql query is '{}']", __TIME__, __FILE__, __LINE__, query);
    int ret = mysql_real_query(this->m_handle, query.c_str(), query.size());
    if (ret)
    {
        spdlog::error("[{}:{}] [msyql_real_query error: '{}']", __TIME__, __FILE__, __LINE__, mysql_error(this->m_handle));
        return -1;
    }
    return 0;
}

int 
galay::MiddleWare::MySql::SyncMySql::ModFieldName(const std::string& TableName,const std::string& OldFieldName, const std::pair<std::string,std::string>& Field_Type)
{
    if(TableName.empty() || OldFieldName.empty() ){
        spdlog::error("[{}:{}] [Arg: [TableName] of [OldFieldName] empty]", __TIME__, __FILE__, __LINE__);
        return -1;
    }
    std::string query = "ALTER TABLE " + TableName + " CHANGE COLUMN " + OldFieldName + " " + Field_Type.first + " " + Field_Type.second + ';';
    if (mysql_ping(this->m_handle))
    {
        spdlog::error("[{}:{}] [mysql_ping error: '{}']", __TIME__, __FILE__, __LINE__, mysql_error(this->m_handle));
        return -1;
    }
    spdlog::info("[{}:{}] [mysql query is '{}']", __TIME__, __FILE__, __LINE__, query);
    int ret = mysql_real_query(this->m_handle, query.c_str(), query.size());
    if (ret)
    {
        spdlog::error("[{}:{}] [msyql_real_query error: '{}']", __TIME__, __FILE__, __LINE__, mysql_error(this->m_handle));
        return -1;
    }
    return 0;
}


int 
galay::MiddleWare::MySql::SyncMySql::DelField(const std::string& TableName,const std::string& FieldName)
{
    if(TableName.empty() || FieldName.empty()){
        spdlog::error("[{}:{}] [Arg: [TableName] of [OldFieldName] empty]", __TIME__, __FILE__, __LINE__);
        return -1;
    }
    std::string query = "ALTER TABLE " + TableName + "DROP COLUMN " + FieldName + ";";
    if (mysql_ping(this->m_handle))
    {
        spdlog::error("[{}:{}] [mysql_ping error: '{}']", __TIME__, __FILE__, __LINE__, mysql_error(this->m_handle));
        return -1;
    }
    spdlog::info("[{}:{}] [mysql query is '{}']", __TIME__, __FILE__, __LINE__, query);
    int ret = mysql_real_query(this->m_handle, query.c_str(), query.size());
    if (ret)
    {
        spdlog::error("[{}:{}] [msyql_real_query error: '{}']", __TIME__, __FILE__, __LINE__, mysql_error(this->m_handle));
        return -1;
    }
    return 0;
}

// 发送二进制数据
int 
galay::MiddleWare::MySql::SyncMySql::ParamSendBinaryData(const std::string &ParamQuery, const std::string& Data)
{
    if (Data.empty() || ParamQuery.empty())
    {
        spdlog::error("[{}:{}] [Arg: [ParamQuery] or [buffer] empty]", __TIME__, __FILE__, __LINE__);
        return -1;
    }
    if (mysql_ping(this->m_handle))
    {
        spdlog::error("[{}:{}] [mysql_ping error: '{}']", __TIME__, __FILE__, __LINE__, mysql_error(this->m_handle));
        return -1;
    }
    MYSQL_STMT *stmt = mysql_stmt_init(this->m_handle);
    if(stmt == nullptr) {
        spdlog::error("[{}:{}] [mysql_stmt_init error: '{}']", __FILE__, __LINE__, mysql_error(this->m_handle));
        return -1;
    }
    spdlog::info("[{}:{}] [mysql query is '{}']", __TIME__, __FILE__, __LINE__,ParamQuery);
    int ret = mysql_stmt_prepare(stmt, ParamQuery.c_str(), ParamQuery.length());
    if (ret)
    {
        spdlog::error("[{}:{}] [mysql_stmt_prepare error: '{}']", __TIME__, __FILE__, __LINE__, mysql_error(this->m_handle));
        return -1;
    }
    MYSQL_BIND param = {0};
    param.buffer_type = MYSQL_TYPE_LONG_BLOB;
    param.buffer = NULL;
    param.is_null = 0;
    param.length = NULL;
    ret = mysql_stmt_bind_param(stmt, &param);
    if (ret)
    {
        spdlog::error("[{}:{}] [mysql_stmt_bind_param error: '{}']", __TIME__, __FILE__, __LINE__, mysql_error(this->m_handle));
        return -1;
    }
    ret = mysql_stmt_send_long_data(stmt, 0, Data.c_str(), Data.length());
    if (ret)
    {
        spdlog::error("[{}:{}] [mysql_stmt_send_long_data error: '{}']", __TIME__, __FILE__, __LINE__, mysql_error(this->m_handle));
        return -1;
    }
    ret = mysql_stmt_execute(stmt);
    if (ret)
    {
        spdlog::error("[{}:{}] [mysql_stmt_execute error: '{}']", __TIME__, __FILE__, __LINE__, mysql_error(this->m_handle));
        return -1;
    }
    ret = mysql_stmt_close(stmt);
    if (ret)
    {
        spdlog::error("[{}:{}] [mysql_stmt_close error: '{}']", __TIME__, __FILE__, __LINE__, mysql_error(this->m_handle));
        return -1;
    }
    return 0;
}
// 接收二进制数据
int
galay::MiddleWare::MySql::SyncMySql::ParamRecvBinaryData(const std::string &ParamQuery, std::string& Data)
{
    if (ParamQuery.empty())
    {
        spdlog::error("[{}:{}] [Arg: [ParamQuery] is empty]", __TIME__, __FILE__, __LINE__);
        return -1;
    }
    if(!Data.empty()){
        spdlog::warn("[{}:{}] [Arg: [Data] is not empty , auto clear it]", __TIME__, __FILE__, __LINE__);
        Data.clear();
    }
    if (mysql_ping(this->m_handle))
    {
        spdlog::error("[{}:{}] [mysql_ping error: '{}']", __TIME__, __FILE__, __LINE__, mysql_error(this->m_handle));
        return -1;
    }
    MYSQL_STMT *stmt = mysql_stmt_init(this->m_handle);
    if(stmt == nullptr) {
        spdlog::error("[{}:{}] [mysql_stmt_init error: '{}']", __FILE__, __LINE__, mysql_error(this->m_handle));
        return -1;
    }
    spdlog::info("[{}:{}] [mysql query is '{}']", __TIME__, __FILE__, __LINE__,ParamQuery);
    int ret = mysql_stmt_prepare(stmt, ParamQuery.c_str(), ParamQuery.length());
    if (ret)
    {
        spdlog::error("[{}:{}] [mysql_stmt_prepare error: '{}']", __FILE__, __LINE__, mysql_error(this->m_handle));
        return -1;
    }
    MYSQL_BIND result = {0};
    result.buffer_type = MYSQL_TYPE_LONG_BLOB;
    uint64_t total_length = 0;
    result.length = &total_length;

    ret = mysql_stmt_bind_result(stmt, &result);
    if (ret)
    {
        spdlog::error("[{}:{}] [mysql_stmt_bind_result error: '{}']", __FILE__, __LINE__, mysql_error(this->m_handle));
        return -1;
    }

    ret = mysql_stmt_execute(stmt);
    if (ret)
    {
        spdlog::error("[{}:{}] [mysql_stmt_execute error: '{}']", __FILE__, __LINE__, mysql_error(this->m_handle));
        return -1;
    }

    ret = mysql_stmt_store_result(stmt);
    if (ret)
    {
        spdlog::error("[{}:{}] [mysql_stmt_store_result error: '{}']", __FILE__, __LINE__, mysql_error(this->m_handle));
        return -1;
    }
    char buffer[1024] = {0};
    while (1)
    {
        ret = mysql_stmt_fetch(stmt);
        if (ret != 0 && ret != MYSQL_DATA_TRUNCATED){
            spdlog::info("[{}:{}] [mysql_stmt_store_result success]", __FILE__, __LINE__);
            break; // 异常
        }
        long long save = 0;
        while (save < total_length)
        {
            bzero(buffer,1024);
            result.buffer = buffer;
            result.buffer_length = total_length - save >= 1024 ? 1024 : total_length - save;
            mysql_stmt_fetch_column(stmt, &result, 0, save);
            save += result.buffer_length;
            Data += std::string(reinterpret_cast<char*>(result.buffer),result.buffer_length);
        }
    }
    mysql_stmt_close(stmt);
    return 0;
}


bool 
galay::MiddleWare::MySql::SyncMySql::StartTransaction()
{
    if (mysql_ping(this->m_handle))
    {
        return -1;
    }
    spdlog::info("[{}:{}] [mysql query is 'start transaction']");
    if(mysql_real_query(this->m_handle,"start transaction;",18))
    {
        spdlog::error("[{}:{}] [msyql_real_query error: '{}']",__FILE__,__LINE__,mysql_error(this->m_handle));
        return false;
    }
    return true;
}

bool 
galay::MiddleWare::MySql::SyncMySql::Commit()
{
    if (mysql_ping(this->m_handle))
    {
        return -1;
    }
    spdlog::info("[{}:{}] [mysql query is 'commit']");
    if(mysql_real_query(this->m_handle,"commit;",7))
    {
        spdlog::error("[{}:{}] {}",__FILE__,__LINE__,mysql_error(this->m_handle));
        return false;
    }
    return true;
}

bool 
galay::MiddleWare::MySql::SyncMySql::Rollback()
{
    if (mysql_ping(this->m_handle))
    {
        return -1;
    }
    spdlog::info("[{}:{}] [mysql query is 'rollback']");
    if(mysql_real_query(this->m_handle,"rollback;",9))
    {
        spdlog::error("[{}:{}] {}",__FILE__,__LINE__,mysql_error(this->m_handle));
        return false;
    }
    return true;
}

galay::MiddleWare::MySql::SyncMySql::~SyncMySql()
{
    if(this->m_connected) {
        mysql_close(this->m_handle); // 关闭数据库连接
        spdlog::info("[{}:{}] [mysql connection close]",__FILE__,__LINE__);
        this->m_connected = false;
    }else{
        spdlog::warn("[{}:{}] [mysql connection is reaptedly close]",__FILE__,__LINE__);
    }
}