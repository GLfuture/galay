#include <iostream>
#include <string>
#include <regex>

// void ParseMysqlUrl(const std::string &url) {
//     // 简化并修正正则表达式
//     std::regex re(R"(mysql://([^:@]+)(?::([^@]+))?@([^:/?]+)(?::(\d+))?(?:/([^?]*))?)");
//     std::smatch matches;

//     if (std::regex_search(url, matches, re)) {
//         if (matches[1].matched) {
//             std::cout << "username: " << matches[1].str() << std::endl;
//         }
//         if (matches[2].matched) {
//             std::cout << "password: " << matches[2].str() << std::endl;
//         }
//         if (matches[3].matched) {
//             std::cout << "host: " << matches[3].str() << std::endl;
//         }
//         if (matches[4].matched) {
//             std::cout << "port: " << matches[4].str() << std::endl;
//         }
//         if (matches[5].matched) {
//             std::cout << "database: " << matches[5].str() << std::endl;
//         }
//     } else {
//         std::cerr << "URL format is incorrect." << std::endl;
//     }
// }

// int main() {
//     try {
//         ParseMysqlUrl("mysql://root:123456@127.0.0.1:3306/test");
//     } catch (const std::exception& e) {
//         std::cerr << e.what() << std::endl;
//     }
//     try {
//         ParseMysqlUrl("mysql://root:123456@127.0.0.1/test");
//     } catch (const std::exception& e) {
//         std::cerr << e.what() << std::endl;
//     }
    
//     return 0;
// }

//#include <galay/middleware/Mysql.hpp>
#include <iostream>
#include <galay/galay.h>

//using namespace galay::mysql;
using namespace galay;


Coroutine<void> test()
{
    co_return;
}

int main()
{
    // MysqlSelectQuery select;
    // select.Fields("*").From("test").FullJoinOn("mvp", "mvp.id = test.id").Where("id > 1").GroupBy("id").OrderBy(FieldOrderByPair("id", MysqlOrderBy::ASC))\
    //     .Limit(100);
    // std::cout << select.ToString() << std::endl;
    // MysqlInsertQuery insert;
    // insert.Table("test").FieldValues(FieldValuePair("id", "1"), FieldValuePair("name", "\"gong\""));
    // std::cout << insert.ToString() << std::endl;
    // MysqlUpdateQuery update;
    // update.Table("test").FieldValues(FieldValuePair("id", "1"), FieldValuePair("name", "\"gong\"")).Where("id > 1");
    // std::cout << update.ToString() << std::endl;
    // MysqlField field;
    // field.Name("comment").Type("varchar(10)").NotNull().Default("\"hello\"");
    // MysqlAlterQuery alter;
    // alter.Table("test").AddColumn(field);
    // std::cout << alter.ToString() << std::endl;
    // return 0;
    InitializeGalayEnv({1, -1}, {1, -1}, {1, -1});
    std::cout << std::boolalpha << test().Done() << std::endl;
    std::cout << sizeof(std::weak_ptr<std::atomic_bool>) << std::endl;
    std::cout << sizeof(std::shared_ptr<Coroutine<void>>) << std::endl;
    std::cout << sizeof(galay::protocol::dns::DnsFlags) << std::endl;
    //std::cout << sizeof(galay::protocol::http::Http2FrameHeader) << std::endl;
    getchar();
    DestroyGalayEnv();
    return 0;
}