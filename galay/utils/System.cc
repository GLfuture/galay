#include "galay/kernel/Log.h"
#include "System.h"
#include <filesystem>
#include <fstream>
#include <chrono>
#include <cstdlib>

#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__OPenBSD__) || defined(__NetBSD__)
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#elif defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

namespace galay::utils
{

int64_t GetCurrentTimeMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

std::string GetCurrentGMTTimeString()
{
    std::time_t now = std::time(nullptr);
    std::tm *gmt_time = std::gmtime(&now);
    if (gmt_time == nullptr)
    {
        return "";
    }
    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", gmt_time);
    return buffer;
}

std::string 
ReadFile(const std::string& FileName, bool IsBinary)
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
WriteFile(const std::string& FileName,const std::string& Content, bool IsBinary)
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
ZeroReadFile(const std::string &FileName)
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
ZeroWriteFile(const std::string &FileName, const std::string &Content ,bool IsBinary)
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
    memcpy(p, Content.c_str(), Content.size());
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

std::string GetEnvValue(const std::string &name)
{
    return std::getenv(name.c_str());;
}

std::string GetHostIPV4(const std::string &domain) {
    struct addrinfo hints, *res, *p;
    int status;
    char ipstr[INET_ADDRSTRLEN];
    std::string result;


    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // 只获取IPv4地址
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(domain.c_str(), NULL, &hints, &res)) != 0) {
        return "";
    }

    // 遍历地址列表，取第一个IPv4地址
    for (p = res; p != NULL; p = p->ai_next) {
        void *addr;
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
        addr = &(ipv4->sin_addr);

        // 将地址转换为字符串
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        result = ipstr;
        break; // 取第一个地址
    }

    freeaddrinfo(res);
    return result;
}


// 辅助函数：检查字符串是否由合法域名字符组成
bool isDomainChar(char c) {
    return std::isalnum(c) || c == '-' || c == '.';
}

// 域名格式验证
bool isValidDomain(const std::string& domain) {
    if (domain.empty() || domain.size() > 253) return false;
    if (domain.front() == '.' || domain.back() == '.') return false;

    size_t label_start = 0;
    bool last_dot = false;

    for (size_t i = 0; i < domain.size(); ++i) {
        if (domain[i] == '.') {
            if (last_dot || (i - label_start) < 1 || (i - label_start) > 63)
                return false;
            label_start = i + 1;
            last_dot = true;
        } else {
            if (!isDomainChar(domain[i]) || 
                (domain[i] == '-' && (i == label_start || i == domain.size() - 1)))
                return false;
            last_dot = false;
        }
    }

    size_t last_label = domain.size() - label_start;
    return (last_label >= 1 && last_label <= 63);
}

// 主判断函数
AddressType CheckAddressType(const std::string& input) {
    struct sockaddr_in sa4;
    struct sockaddr_in6 sa6;

    // 检测 IPv4
    if (inet_pton(AF_INET, input.c_str(), &sa4.sin_addr) == 1) {
        // 额外验证格式（防止类似 "1.2.3.4.5" 被误判）
        std::string normalized(input.size(), 0);
        inet_ntop(AF_INET, &sa4.sin_addr, &normalized[0], input.size() + 1);
        if (normalized == input) return AddressType::IPv4;
    }

    // 检测 IPv6
    if (inet_pton(AF_INET6, input.c_str(), &sa6.sin6_addr) == 1) {
        // 标准化验证（处理缩写）
        std::string normalized(INET6_ADDRSTRLEN, 0);
        inet_ntop(AF_INET6, &sa6.sin6_addr, &normalized[0], INET6_ADDRSTRLEN);
        normalized.resize(strlen(normalized.c_str()));
        return (normalized == input) ? AddressType::IPv6 : AddressType::Invalid;
    }

    // 检测域名
    return isValidDomain(input) ? AddressType::Domain : AddressType::Invalid;
}



}