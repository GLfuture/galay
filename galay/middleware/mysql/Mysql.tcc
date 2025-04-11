#ifndef GALAY_MYSQL_TCC
#define GYLAY_MYSQL_TCC

#include "Mysql.hpp"

namespace galay::mysql
{

template <FieldNameType... Field>
inline MysqlSelectQuery &MysqlSelectQuery::Fields(Field... fields)
{
    m_stream << "SELECT ";
    bool first = true;
    (..., ([this, &first, &fields]() {
        if constexpr (sizeof...(Field) == 0) {
            return;
        } 
        if(!first) m_stream << ',';
        m_stream << fields;
        first = false;
    }()));
    return *this;
}

template <TableNameType... Table>
inline MysqlSelectQuery &MysqlSelectQuery::From(Table... tables)
{
    m_stream << " FROM ";
    bool first = true;
    (..., ([this, &first, &tables]() {
        if constexpr (sizeof...(Table) == 0) {
            return;
        } 
        if(!first) m_stream << ',';
        m_stream << tables;
        first = false;
    }()));
    return *this;
}

template <FieldNameType... Field>
inline MysqlSelectQuery &MysqlSelectQuery::GroupBy(Field... fields)
{
    m_stream << " GROUP BY ";
    bool first = true;
    (..., ([this, &first, &fields]() {
        if constexpr (sizeof...(Field) == 0) {
            return;
        } 
        if(!first) m_stream << ',';
        m_stream << fields;
        first = false;
    }()));
    return *this;
}


template <OrderByType ...OrderType>
inline MysqlSelectQuery &MysqlSelectQuery::OrderBy(OrderType... orders)
{
    m_stream << " ORDER BY ";
    bool first = true;
    (..., ([this, &first, &orders]() {
        if constexpr (sizeof...(OrderType) == 0) {
            return;
        } 
        if(!first) m_stream << ',';
        m_stream << orders.first << ' ' << OrderByToString(orders.second);
        first = false;
    }()));
    return *this;
}

template <FieldValueType... FieldsValue>
inline MysqlInsertQuery &MysqlInsertQuery::FieldValues(FieldsValue... values)
{
    m_stream << "(";
    bool first = true;
    (..., ([this, &first, &values]() {
        if constexpr (sizeof...(FieldsValue) == 0) {
            return;
        } 
        if(!first) m_stream << ',';
        m_stream << std::get<0>(values);
        first = false;
    }()));
    m_stream << ") VALUES (";
    first = true;
    (..., ([this, &first, &values]() {
        if constexpr (sizeof...(FieldsValue) == 0) {
            return;
        }
        if(!first) m_stream << ',';
        m_stream << std::get<1>(values);
        first = false;
    }()));
    m_stream << ")";
    return *this;
}

template <FieldValueType... FieldsValue>
inline MysqlUpdateQuery &MysqlUpdateQuery::FieldValues(FieldsValue... values)
{
    m_stream << " SET ";
    bool first = true;
    (..., ([this, &first, &values]() {
        if(!first) m_stream << ',';
        m_stream << values.first << "=" << values.second;
        first = false;
    }()));
    return *this;
}

template<typename CoRtn>
AsyncResult<bool, CoRtn> 
AsyncMysqlSession::AsyncConnect(const std::string &host, const std::string &username, const std::string &password, const std::string &db_name, uint32_t port)
{
    net_async_status status = mysql_real_connect_nonblocking(this->m_mysql, host.c_str(), username.c_str(), password.c_str(), db_name.c_str(), port, nullptr, 0);
    if (status )
    {
    }

    return {true};
}

}

#endif