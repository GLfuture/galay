#include "middleware/Mysql.h"
#include "galay.h"
#include <iostream>
#include <fstream>
#include <filesystem>


int main()
{
    galay::middleware::mysql::MysqlClient mysqlclient;
    mysqlclient.Connect("127.0.0.1","gong","123456","test",3306);
    // std::cout << mysqlclient.GetSocket() << '\n';
    // mysqlclient.CreateTable("user",{{"name","varchar(10)","NOT NULL PRIMARY KEY"},{"age","int",""}});
    // auto res = mysqlclient.Select("user",{"name","age"});
    // for(auto &v: res){
    //     for(auto &c: v) std::cout << c <<" ";
    //     std::cout << '\n';
    // }
    // mysqlclient.CreateTable("images2",{{"uid","int","primary key"},{"image","longblob","not null"},{"image2","longblob","not null"}});
    // std::string indata1 = galay::io::file::SyncFileStream::ReadFile("out1.jpeg");
    // auto params = mysqlclient.GetMysqlParams();
    // params->Prepare("insert into images2(uid,image,image2) values (15,?,?);");
    // galay::middleware::mysql::MysqlStmtValue value1,value2;
    // value1.SetValue( galay::middleware::mysql::MysqlStmtValue::BlobType::kBlobTypeLongBlob, indata1);
    // value2.SetValue( galay::middleware::mysql::MysqlStmtValue::BlobType::kBlobTypeLongBlob, indata1);
    // std::vector<galay::middleware::mysql::MysqlStmtValue> values = {value1, value2};
    // params->BindParam(values);
    // params->StringToParam(value1, 0);
    // params->StringToParam(value2, 1);
    // params->Execute();
    // params->Close();
    // std::string outdata;
    // mysqlclient.ParamRecvBinaryData("select image from images2 where uid = 15;",outdata);
    // galay::io::file::SyncFileStream::WriteFile("out3.jpeg",outdata);
    // mysqlclient.DropTable("user");
    // mysqlclient.DropTable("images2");
    auto executor = mysqlclient.GetMysqlParams();
    executor->Prepare("select uid from images where uid > 1;");
    MYSQL_BIND binds[2] = {0};
    unsigned long resLen0 = 0,resLen1 = 0;
    binds[0].buffer_type = MYSQL_TYPE_LONG;
    int uid = 0;
    bool error1,error2;
    binds[0].buffer = &uid;
    binds[0].length = &resLen0;
    binds[0].error = &error1;
    binds[1].buffer_type = MYSQL_TYPE_LONG_BLOB;
    binds[1].length = &resLen1;
    binds[1].buffer_length = 0;
    binds[1].buffer = nullptr;
    binds[1].error = &error2;
    executor->BindResult(binds);
    executor->Execute();
    std::vector<std::string> res;
    while(executor->GetARow(binds)){
        std::cout << uid << '\n';
    }
    std::cout << error1 << '\n';
    std::cout << error2 << '\n';
    std::cout << res.size() << '\n'; 
    mysqlclient.DisConnect();
    return 0;
}