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
    auto sel_query = MysqlSelectQuery();
    sel_query.From("user").Fields("name","age").Where("uid==0");
    auto res = session->Select(sel_query);
    auto res_table = res.ToResultTable();
    for(auto &v: res_table){
        for(auto &c: v) std::cout << c <<" ";
        std::cout << '\n';
    }
}

void CreateImageTable(MysqlSession* session)
{
    MysqlTable table("images");
    table.PrimaryKeyFiled().Name("uid").AutoIncrement().Type("int");
    table.SimpleFiled().Name("image").Type("longblob").NotNull();
    session->CreateTable(table);

    std::string indata = galay::utils::ReadFile("1.1.png");
    auto params = session->GetMysqlStmtExecutor();
    params->Prepare("insert into images(uid,image) values (15,?);");
    MYSQL_BIND binds1[1];
    binds1[0].buffer_type = MYSQL_TYPE_LONG_BLOB;
    binds1[0].buffer = indata.data();
    binds1[0].buffer_length = indata.length();
    params->BindParam(binds1);
    params->LongDataToParam(indata, 0);
    params->Execute();
    params->Close();
    auto executor = session->GetMysqlStmtExecutor();
    executor->Prepare("select image from images where uid > 1;");
    int uid = 0;
    bool error1;
    unsigned long resLen0 = 0;
    MYSQL_BIND binds[1] = {0};
    binds[0].buffer_type = MYSQL_TYPE_LONG_BLOB;
    binds[0].length = &resLen0;
    binds[0].error = &error1;
    executor->BindResult(binds);
    executor->Execute();
    std::vector<std::string> res;
    while(executor->GetARow(binds)){
        
    }
    delete[] (char*)binds[0].buffer;
    std::string outpng((char*)binds[0].buffer, resLen0);
    galay::utils::WriteFile("out.png", outpng);

}

int main()
{
    auto config = galay::mysql::MysqlConfig::CreateConfig();
    MysqlClient<MysqlSession> mysqlclient(config);
    mysqlclient.GetSession()->Connect("127.0.0.1","gong","123456","test",3306);
    CreateUserTable(mysqlclient.GetSession());
    CreateImageTable(mysqlclient.GetSession());
    mysqlclient.GetSession()->DropTable("user");
    mysqlclient.GetSession()->DropTable("images");
    mysqlclient.GetSession()->DisConnect();
    return 0;
}