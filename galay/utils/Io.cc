#include "Io.h"
#include "kernel/Log.h"
#include <filesystem>
#include <fstream>

#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__OPenBSD__) || defined(__NetBSD__)
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/mman.h>
#endif


namespace galay::io::file
{
std::string 
SyncFileStream::ReadFile(const std::string& FileName, bool IsBinary)
{
    std::ifstream in;
    if(IsBinary){
        in.open(FileName, std::ios::binary);
    }else{
        in.open(FileName);
    }
    if(in.fail()) {
        LogError("[Open file error, filename:{}]", FileName);
        return "";
    }
    uintmax_t size = std::filesystem::file_size(FileName);
    char *buffer = new char[size];
    in.read(buffer, size);
    LogTrace("[Read from file: {} Bytes]", size);
    in.close();
    std::string res(buffer,size);
    delete[] buffer;
    return res;
}

void 
SyncFileStream::WriteFile(const std::string& FileName,const std::string& Content, bool IsBinary)
{
    std::ofstream out;
    if(IsBinary){
        out.open(FileName, std::ios::binary);
    }else{
        out.open(FileName);
    }
    if(out.fail()){
        std::string path = std::filesystem::current_path();
        LogError("[Open file: {} failed , now path is {}]", FileName, path);
        return ;
    }
    LogTrace("[Write to file: {} Bytes]", Content.length());
    out.write(Content.c_str(),Content.length());
    out.close();
}

#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__)
std::string 
ZeroCopyFile::ReadFile(const std::string &FileName)
{
    int fd = open(FileName.c_str(),O_RDONLY);
    if(fd == -1){
        std::string path = std::filesystem::current_path();
        LogError("[Open file: {} failed , now path is {}]", FileName, path);
        return "";
    }
    struct stat st;
    if(fstat(fd,&st) == -1){
        LogError("[Fstat file: {} failed]", FileName);
        close(fd);
        return "";
    }
    char *p = reinterpret_cast<char*>(mmap(nullptr,st.st_size,PROT_READ,MAP_PRIVATE,fd,0));
    if( p == MAP_FAILED) 
    {
        LogError("[mmap file: {} failed]", FileName);
        close(fd);
        return "";
    }
    std::string res(p,st.st_size);
    munmap(p,st.st_size);
    close(fd);
    return std::move(res);
}


void 
ZeroCopyFile::WriteFile(const std::string &FileName, const std::string &Content,bool IsBinary)
{
    int fd = open(FileName.c_str(),O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if(fd == -1) {
        std::string path = std::filesystem::current_path();
        LogError("[Open file: {} failed, now path is {}]", FileName,path);
        return;
    }
    if(ftruncate(fd,Content.size()) == -1){
        LogError("[ftruncate {} fail] [size:{} Bytes]", FileName, Content.length());
        return;
    }
    char* p = reinterpret_cast<char*>(mmap(nullptr,Content.size(),PROT_READ | PROT_WRITE,MAP_SHARED,fd,0));
    if (p == MAP_FAILED) {
        LogError("[mmap {} fail]", FileName);
        return;
    }
    memcpy(p,Content.c_str(),Content.size());
    if (msync(p,Content.size(),MS_SYNC) == -1) {
        LogError("[msync {} fail]", FileName);
        return;
    }
    if (munmap(p,Content.size()) == -1) {
        LogError("[munmap {} fail]", FileName);
        return;
    }
    close(fd);
}
#endif
}