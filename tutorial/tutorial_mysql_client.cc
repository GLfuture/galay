#include "galay/middleware/mysql/Mysql.hpp"
#include "galay/galay.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>

using namespace galay::mysql;

void CreateUserTable(MysqlSession* session)
{
    MysqlTable table("user");
    table.PrimaryKeyFiled().Name("uid").Type("int").AutoIncrement();
    table.SimpleFiled().Name("name").Type("varchar(20)").NotNull();
    table.SimpleFiled().Name("age").Type("int").NotNull();
    session->CreateTable(table);
    auto ins_query = MysqlInsertQuery();
    ins_query.Table("user").FieldValues(std::pair{ "name", "\"zhangsan\"" }, std::pair{ "age", "18" });
    session->Insert(ins_query);
    auto sel_query = MysqlSelectQuery();
    sel_query.Fields("name","age").From("user").Where("uid=1");
    auto res = session->Select(sel_query);
    auto res_table = res.ToResultTable();
    for(auto &v: res_table){
        for(auto &c: v) std::cout << c <<" ";
        std::cout << '\n';
    }
    MysqlUpdateQuery upd_query;
    upd_query.Table("user").FieldValues(std::pair{"name", "\"lisi\""}).Where("uid=1");
    session->Update(upd_query);
}

void CreateImageTable(MysqlSession* session)
{
    MysqlTable table("images");
    table.PrimaryKeyFiled().Name("image_id").Type("int").AutoIncrement();
    table.ForeignKeyField("user", "uid").Name("uid").Type("int");
    table.SimpleFiled().Name("image").Type("longblob").NotNull();
    session->CreateTable(table);

    std::string indata = galay::utils::ReadFile("1.1.png");
    auto params = session->GetMysqlStmtExecutor();
    params->Prepare("insert into images(uid,image) values (1,?);");
    MYSQL_BIND binds1[1];
    memset(&binds1[0], 0, sizeof(MYSQL_BIND)); // 清零初始化
    binds1[0].buffer_type = MYSQL_TYPE_LONG_BLOB; // 仅设置类型
    params->BindParam(binds1);
    params->LongDataToParam(indata, 0);
    params->Execute();
    params->Close();
    auto executor = session->GetMysqlStmtExecutor();
    executor->Prepare("select image from images where uid = 1;");
    int uid = 0;
    bool error1;
    unsigned long resLen0 = 0;
    MYSQL_BIND binds[1] = {0};
    executor->Execute();
    executor->StoreResult();
    executor->BindResult();
    std::vector<std::string> res;
    int i = 0, j = 0;
    while(executor->NextRow(i)){
        while(executor->NextColumn(j)) {
            if(executor->IsNull(j)) {
                std::cout << "null ";
            } else if(executor->IsBlob(j)) {
                galay::utils::WriteFile("out.png", galay::utils::uint8ToString(executor->GetBlob(j)));
            }
        }
        std::cout << std::endl;
    }

}

int main()
{
    auto config = galay::mysql::MysqlConfig::CreateConfig();
    //MysqlClient<MysqlSession> mysqlclient(config);
    // MYSQL* mysql = mysql_init(nullptr);
    // if(mysql_real_connect(mysql, "127.0.0.1", "gong", "123456", "test", 3306, nullptr, 0) == nullptr) {
    //     std::cout << mysql_error(mysql) << std::endl;
    //     return 0;
    // }
    std::cout << "mysql version: " << mysql_get_client_info() << std::endl;
    try
    {
        MysqlSession session(config);
        session.Connect("127.0.0.1", "gong", "123456", "test", 3306);
        // 或者 session.Connect("mysql://user:password@host:port/db_name")
        CreateUserTable(&session);
        CreateImageTable(&session);
        MysqlDeleteQuery del_query;
        del_query.Table("user").Where("uid=1");
        session.Delete(del_query);
        // 错误drop顺序
        // auto res = session.DropTable("user");
        // if(!res.Success()) {
        //     std::cout << res.GetError() << std::endl;
        // }
        // session.DropTable("images");
        // 正确drop顺序
        session.DropTable("images");
        session.DropTable("user");
        session.DisConnect();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    return 0;
}