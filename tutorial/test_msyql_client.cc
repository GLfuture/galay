#include "../galay/middleware/SyncMySql.h"
#include "../galay/util/file.h"
#include <fstream>
#include <filesystem>


int main()
{
    galay::MiddleWare::SyncMySql mysqlclient;
    mysqlclient.Connect("127.0.0.1","gong","123456","test",3306);
    mysqlclient.CreateTable("user",{{"name","varchar(10)","NOT NULL PRIMARY KEY"},{"age","int",""}});
    //mysqlclient.Insert("user",{{"name","'gong'"},{"age","10"}});
    auto res = mysqlclient.Select("user",{"name","age"});
    for(auto &v: res){
        for(auto &c: v) std::cout << c <<" ";
        std::cout << '\n';
    }
    mysqlclient.CreateTable("images",{{"uid","int","primary key"},{"image","longblob","not null"}});
    std::string indata1 = galay::FileOP::ReadFile("1.jpeg") , indata2 = galay::FileOP::ReadFile("2.jpeg");
    std::cout << indata1.length() << " " << indata2.length() << '\n';
    mysqlclient.ParamSendBinaryData("insert into images(uid,image) values (1,?);",indata1);
    mysqlclient.ParamSendBinaryData("insert into images(uid,image) values (2,?);",indata2);
    std::string outdata;
    mysqlclient.ParamRecvBinaryData("select image from images where uid = 2;",outdata);
    galay::FileOP::WriteFile("out1.jpeg",outdata);
    return 0;
}