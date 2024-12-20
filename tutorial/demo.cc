#include "galay/galay.h"
#include "galay/middleware/Mysql.hpp"
#include <iostream>

using namespace galay::mysql;

int main()
{
    // auto config = MysqlConfig::CreateConfig();
    // config->WithCharset("utf8mb4").WithConnectTimeout(1000).WithReadTimeout(1000).WithWriteTimeout(1000);
    // MysqlSession seesion(config);
    // seesion.Connect("127.0.0.1", "gong", "123456", "test");
    MysqlTable table("user");
    table.PrimaryKey().Name("uid").Type("INT").AutoIncrement().Comment("用户id");
    table.ForeignKey("images(uid)").Name("imageId").Type("INT").NotNull().Comment("图片id");
    std::cout << table.ToString() << std::endl;
    //seesion.CreateTable(table);
    return 0;
}