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

MysqlField &MysqlTable::PrimaryKeyFiled()
{
    m_primary_keys.emplace_back();
    return m_primary_keys.back();
}

MysqlField &MysqlTable::ForeignKeyField(const std::string &table_name, const std::string& field_name)
{
    m_foreign_keys.emplace_back(MysqlField{}, table_name + "(" + field_name + ")");
    return m_foreign_keys.back().first;
}

MysqlField &MysqlTable::CreateUpdateTimeStampField()
{
    m_other_fields.emplace_back();
    m_other_fields.back().Type("TIMESTAMP").Default("CURRENT_TIMESTAMP");
    return m_other_fields.back();
}

MysqlField &MysqlTable::SimpleFiled()
{
    m_other_fields.emplace_back();
    return m_other_fields.back();
}

MysqlTable &MysqlTable::ExtraAction(const std::string &action)
{
    m_extra_action += action;
    return *this;
}

std::string MysqlTable::ToString() const
{
    std::ostringstream query, constraints;
    query << "CREATE TABLE IF NOT EXISTS " << m_name << "(";

    // 添加所有字段（主键、外键字段视为普通字段）
    for (const auto& pk : m_primary_keys) {
        query << pk.ToString() << ",";
    }
    for (const auto& fk : m_foreign_keys) {
        query << fk.first.ToString() << ",";
    }
    for (const auto& field : m_other_fields) {
        query << field.ToString() << ",";
    }

    // 构建主键约束
    if (!m_primary_keys.empty()) {
        constraints << "PRIMARY KEY(";
        for (size_t i = 0; i < m_primary_keys.size(); ++i) {
            if (i > 0) {
                constraints << ",";
            }
            constraints << m_primary_keys[i].m_name;
        }
        constraints << "),";
    }

    // 构建外键约束
    for (const auto& fk : m_foreign_keys) {
        constraints << "FOREIGN KEY(" << fk.first.m_name << ") REFERENCES "
                    << fk.second << " ON DELETE CASCADE ON UPDATE CASCADE,";
    }

    // 将约束添加到query，并移除末尾的逗号
    std::string constraintStr = constraints.str();
    if (!constraintStr.empty()) {
        if (constraintStr.back() == ',') {
            constraintStr.pop_back();
        }
        query << constraintStr << ","; // 添加约束后加逗号
    }

    // 处理最后的逗号并闭合括号，添加表选项
    std::string final_query = query.str();
    size_t last_comma = final_query.find_last_of(',');
    if (last_comma != std::string::npos) {
        final_query = final_query.substr(0, last_comma) + ") " + m_extra_action + ";";
    } else {
        final_query += ") " + m_extra_action + ";";
    }

    return final_query;
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
    : m_stmt(stmt), m_logger(logger)
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
    return true;
}


bool  
MysqlStmtExecutor::LongDataToParam(const std::string& data, unsigned int index)
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
MysqlStmtExecutor::BindResult()
{
    // 1. 获取结果集元数据
    MYSQL_RES* metadata = mysql_stmt_result_metadata(m_stmt);
    if (!metadata) {
        MysqlLogError(m_logger->SpdLogger(), "[Error: No result metadata]");
        return false;
    }

    const int num_fields = mysql_num_fields(metadata);
    MYSQL_FIELD* fields = mysql_fetch_fields(metadata);
    m_result_binds.resize(num_fields);

    for (int i = 0; i < num_fields; ++i) {
        MYSQL_BIND& bind = m_result_binds[i];
        memset(&bind, 0, sizeof(MYSQL_BIND));

        // 2.1 设置基础类型
        bind.buffer_type = fields[i].type;
        bind.is_null = new bool(false);
        bind.length = new unsigned long(0);

        // 2.2 根据字段类型分配缓冲区
        switch (fields[i].type) {
            // 整数类型
            case MYSQL_TYPE_TINY:
                bind.buffer = new int8_t(0);
                bind.buffer_length = sizeof(int8_t);
                break;
            case MYSQL_TYPE_SHORT:
                bind.buffer = new int16_t(0);
                bind.buffer_length = sizeof(int16_t);
                break;
            case MYSQL_TYPE_LONG:
                bind.buffer = new int32_t(0);
                bind.buffer_length = sizeof(int32_t);
                break;
            case MYSQL_TYPE_LONGLONG:
                bind.buffer = new int64_t(0);
                bind.buffer_length = sizeof(int64_t);
                break;

            // 浮点类型
            case MYSQL_TYPE_FLOAT:
                bind.buffer = new float(0.0f);
                bind.buffer_length = sizeof(float);
                break;
            case MYSQL_TYPE_DOUBLE:
                bind.buffer = new double(0.0);
                bind.buffer_length = sizeof(double);
                break;

            // 字符串和BLOB
            case MYSQL_TYPE_STRING:
            case MYSQL_TYPE_VAR_STRING:
            case MYSQL_TYPE_BLOB: {
                const unsigned long init_size = (fields[i].max_length > 0) ? 
                fields[i].max_length + 1 : 1024;
                bind.buffer = malloc(init_size);
                bind.buffer_length = init_size;
                m_blob_buffers.emplace_back(BlobBuffer{
                    .field_index = i,
                    .data = static_cast<char*>(bind.buffer),
                    .allocated = init_size,
                    .actual = 0
                });
                break;
            }
            default:
            {   
                MysqlLogError(m_logger->SpdLogger(), 
                    "[Error: Unsupported type {} for column {}]", 
                    fields[i].type, fields[i].name);
                mysql_free_result(metadata);
                return false;
            }
        }
    }

    // 3. 绑定结果到预处理语句
    if (mysql_stmt_bind_result(m_stmt, m_result_binds.data()) != 0) {
        MysqlLogError(m_logger->SpdLogger(), "[Error: {}]", mysql_stmt_error(m_stmt));
        mysql_free_result(metadata);
        return false;
    }

    mysql_free_result(metadata);
    return true;
}

bool 
MysqlStmtExecutor::Execute()
{
    if(!m_stmt) {
        MysqlLogError(m_logger->SpdLogger(), "[Error: MysqlStmtExecutor's stmt is nullptr]");
        return false;
    }
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

bool MysqlStmtExecutor::IsNull(int column) const {
    if (column < 0 || column >= m_result_binds.size()) {
        MysqlLogError(m_logger->SpdLogger(), "[Error: Column index out of range]");
        return false;
    }
    const MYSQL_BIND& bind = m_result_binds[column];
    if (!bind.is_null) {
        MysqlLogError(m_logger->SpdLogger(), "[Error: is_null pointer not initialized]");
        return false;
    }
    return (*bind.is_null) != 0;
}

bool MysqlStmtExecutor::IsInt(int column) const {
    ValidateColumn(column, {});
    const auto type = m_result_binds[column].buffer_type;
    return (type == MYSQL_TYPE_TINY || 
            type == MYSQL_TYPE_SHORT || 
            type == MYSQL_TYPE_LONG || 
            type == MYSQL_TYPE_LONGLONG);
}

bool MysqlStmtExecutor::IsString(int column) const {
    ValidateColumn(column, {});
    const auto type = m_result_binds[column].buffer_type;
    return (type == MYSQL_TYPE_STRING || 
            type == MYSQL_TYPE_VAR_STRING);
}

bool MysqlStmtExecutor::IsBlob(int column) const {
    ValidateColumn(column, {});
    const auto type = m_result_binds[column].buffer_type;
    return (type == MYSQL_TYPE_BLOB || 
            type == MYSQL_TYPE_LONG_BLOB);
}

bool MysqlStmtExecutor::IsFloat(int column) const {
    ValidateColumn(column, {});
    const auto type = m_result_binds[column].buffer_type;
    return (type == MYSQL_TYPE_FLOAT || type == MYSQL_TYPE_DOUBLE);
}

bool MysqlStmtExecutor::IsDateTime(int column) const {
    ValidateColumn(column, {});
    const auto type = m_result_binds[column].buffer_type;
    return (type == MYSQL_TYPE_DATE || 
            type == MYSQL_TYPE_TIME || 
            type == MYSQL_TYPE_DATETIME || 
            type == MYSQL_TYPE_TIMESTAMP);
}

int64_t MysqlStmtExecutor::GetInt(int column) const {
    ValidateColumn(column, {MYSQL_TYPE_TINY, MYSQL_TYPE_SHORT, MYSQL_TYPE_LONG, MYSQL_TYPE_LONGLONG});
    if (IsNull(column)) throw std::runtime_error("Column is NULL");

    const MYSQL_BIND& bind = m_result_binds[column];
    switch (bind.buffer_type) {
        case MYSQL_TYPE_TINY:   return *static_cast<int8_t*>(bind.buffer);
        case MYSQL_TYPE_SHORT:  return *static_cast<int16_t*>(bind.buffer);
        case MYSQL_TYPE_LONG:   return *static_cast<int32_t*>(bind.buffer);
        case MYSQL_TYPE_LONGLONG: return *static_cast<int64_t*>(bind.buffer);
        default: throw std::logic_error("Invalid type for integer");
    }
}

double MysqlStmtExecutor::GetDouble(int column) const {
    ValidateColumn(column, {MYSQL_TYPE_FLOAT, MYSQL_TYPE_DOUBLE});
    if (IsNull(column)) throw std::runtime_error("Column is NULL");
    
    const MYSQL_BIND& bind = m_result_binds[column];
    switch (bind.buffer_type) {
        case MYSQL_TYPE_FLOAT:  return *static_cast<float*>(bind.buffer);
        case MYSQL_TYPE_DOUBLE: return *static_cast<double*>(bind.buffer);
        default: throw std::logic_error("Invalid type for double");
    }
}

std::string MysqlStmtExecutor::GetString(int column) const {
    ValidateColumn(column, {MYSQL_TYPE_STRING, MYSQL_TYPE_VAR_STRING});
    if (IsNull(column)) return "";

    const MYSQL_BIND& bind = m_result_binds[column];
    return std::string(static_cast<char*>(bind.buffer), *bind.length);
}

std::vector<uint8_t> MysqlStmtExecutor::GetBlob(int column) const {
    ValidateColumn(column, {MYSQL_TYPE_BLOB, MYSQL_TYPE_LONG_BLOB});
    if (IsNull(column)) return {};

    const MYSQL_BIND& bind = m_result_binds[column];
    const uint8_t* data = static_cast<uint8_t*>(bind.buffer);
    return std::vector<uint8_t>(data, data + *bind.length);
}

MYSQL_TIME MysqlStmtExecutor::GetDateTime(int column) const {
    ValidateColumn(column, {MYSQL_TYPE_DATE, MYSQL_TYPE_TIME, MYSQL_TYPE_DATETIME, MYSQL_TYPE_TIMESTAMP});
    if (IsNull(column)) throw std::runtime_error("Column is NULL");
    
    const MYSQL_BIND& bind = m_result_binds[column];
    return *static_cast<MYSQL_TIME*>(bind.buffer);
}

bool MysqlStmtExecutor::NextRow(int &row)
{
    if (!m_stmt) {
        MysqlLogError(m_logger->SpdLogger(), "[Error: Statement handle is null]");
        return false;
    }
    const int ret = mysql_stmt_fetch(m_stmt);
    if (ret == MYSQL_NO_DATA) {
        return false; // 无更多数据
    } else if (ret == MYSQL_DATA_TRUNCATED) {
        for (int i = 0; i < m_result_binds.size(); ++i) {
            MYSQL_BIND& bind = m_result_binds[i];
            if (bind.buffer_type != MYSQL_TYPE_BLOB) continue;
            const unsigned long actual_length = *bind.length;
            BlobBuffer& blob = m_blob_buffers[i];

            if (actual_length > blob.allocated) {
                free(blob.data);
                blob.data = static_cast<char*>(malloc(actual_length));
                blob.allocated = actual_length;
                bind.buffer = blob.data;
                bind.buffer_length = actual_length;
                if (mysql_stmt_fetch_column(m_stmt, &bind, blob.field_index, 0) != 0) {
                    return false;
                }
            }

            blob.actual = actual_length;
        }
    } else if (ret != 0) {
        MysqlLogError(m_logger->SpdLogger(), "[Error: {}]", mysql_stmt_error(m_stmt));
        return false;
    }
    m_current_row++;
    m_current_col = -1; // 重置列索引，准备遍历新行
    row = m_current_row;
    return true;
}

bool MysqlStmtExecutor::NextColumn(int &column)
{
    if (m_result_binds.empty()) {
        MysqlLogError(m_logger->SpdLogger(), 
            "[Error: Result set not bound]");
        return false;
    }
    m_current_col++;
    const int total_columns = mysql_stmt_field_count(m_stmt);
    if (m_current_col >= total_columns) {
        return false; // 列遍历结束
    }
    column = m_current_col;
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
    for (auto& bind : m_result_binds) {
        // 释放基础指针
        delete bind.is_null;
        delete bind.length;

        // 按类型释放缓冲区
        switch (bind.buffer_type) {
            // 整数类型
            case MYSQL_TYPE_TINY:
                delete static_cast<int8_t*>(bind.buffer);
                break;
            case MYSQL_TYPE_SHORT:
                delete static_cast<int16_t*>(bind.buffer);
                break;
            case MYSQL_TYPE_LONG:
                delete static_cast<int32_t*>(bind.buffer);
                break;
            case MYSQL_TYPE_LONGLONG:
                delete static_cast<int64_t*>(bind.buffer);
                break;

            // 浮点类型
            case MYSQL_TYPE_FLOAT:
                delete static_cast<float*>(bind.buffer);
                break;
            case MYSQL_TYPE_DOUBLE:
                delete static_cast<double*>(bind.buffer);
                break;

            // 字符串和BLOB
            case MYSQL_TYPE_STRING:
            case MYSQL_TYPE_VAR_STRING:
            case MYSQL_TYPE_BLOB:
                free(bind.buffer);
                break;

            default:
                break;
        }
    }
    m_result_binds.clear();

    m_result_binds.clear();

    // 关闭预处理语句
    if (m_stmt) {
        mysql_stmt_close(m_stmt);
        m_stmt = nullptr;
    }
    if(m_stmt) {
        Close();
    }
}

void MysqlStmtExecutor::ValidateColumn(int column, std::initializer_list<enum_field_types> expected_types) const
{
    if (column < 0 || column >= m_result_binds.size()) {
        throw std::out_of_range("Column index out of range");
    }
    if (!expected_types.size()) return;

    const auto actual_type = m_result_binds[column].buffer_type;
    for (auto expected : expected_types) {
        if (actual_type == expected) return;
    }
    throw std::runtime_error("Column type mismatch");
}

MysqlSession::MysqlSession(MysqlConfig::ptr config)
    :MysqlSession(config, Logger::CreateStdoutLoggerMT("MysqlLogger"))
{
}

MysqlSession::MysqlSession(MysqlConfig::ptr config, Logger::ptr logger)
    : m_logger(logger)
{
    mysql_library_init(0, nullptr, nullptr);
    m_mysql = mysql_init(nullptr);
    if(this->m_mysql == nullptr) {
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
    if(m_mysql == nullptr) {
        return false;
    }
    if (!mysql_real_connect(m_mysql, host.c_str(), username.c_str(), password.c_str(), db_name.c_str(), port, nullptr, 0))
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
    mysql_library_end();
}

AsyncMysqlSession::AsyncMysqlSession(MysqlConfig::ptr config)
{
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