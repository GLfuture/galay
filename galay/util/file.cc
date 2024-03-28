#include "file.h"
#include <filesystem>
#include <fstream>
#include <spdlog/spdlog.h>


std::string galay::FileOP::ReadFile(const std::string& FileName)
{
    std::ifstream in(FileName);
    if(in.fail()) {
        spdlog::error("{} {} {} ifstream open file error ",__TIME__,__FILE__,__LINE__);
        return "";
    }
    uintmax_t size = std::filesystem::file_size(FileName);
    char *buffer = new char[size];
    in.read(buffer, size);
    spdlog::info("{} {} {} read file size is {} Bytes",__TIME__,__FILE__,__LINE__,size);
    in.close();
    std::string res(buffer,size);
    delete[] buffer;
    return res;
}

void galay::FileOP::WriteFile(const std::string& FileName,const std::string& Content)
{
    std::ofstream out(FileName);
    if(out.fail()){
        spdlog::error("{} {} {} ifstream open file error ",__TIME__,__FILE__,__LINE__);
        return ;
    }
    spdlog::info("{} {} {} write file content size is {} Bytes",__TIME__,__FILE__,__LINE__,Content.length());
    out.write(Content.c_str(),Content.length());
    out.close();
}