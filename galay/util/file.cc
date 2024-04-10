#include "file.h"
#include <filesystem>
#include <fstream>
#include <spdlog/spdlog.h>


std::string 
galay::Helper::FileOP::ReadFile(const std::string& FileName)
{
    std::ifstream in(FileName);
    if(in.fail()) {
        spdlog::error("[{}:{}] [open file error]",__FILE__,__LINE__);
        return "";
    }
    uintmax_t size = std::filesystem::file_size(FileName);
    char *buffer = new char[size];
    in.read(buffer, size);
    spdlog::info("[{}:{}] [read from file: {} Bytes]",__FILE__,__LINE__,size);
    in.close();
    std::string res(buffer,size);
    delete[] buffer;
    return res;
}

void 
galay::Helper::FileOP::WriteFile(const std::string& FileName,const std::string& Content)
{
    std::ofstream out(FileName);
    if(out.fail()){
        spdlog::error("[{}:{}] [open file error]",__FILE__,__LINE__);
        return ;
    }
    spdlog::info("[{}:{}] [write to file: {} Bytes]",__FILE__,__LINE__,Content.length());
    out.write(Content.c_str(),Content.length());
    out.close();
}